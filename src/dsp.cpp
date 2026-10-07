#include "dsp.hpp"

#include <algorithm>
#include <cmath>

namespace lyra {
namespace {

double parameter(const Effect& effect, const std::string& name, double fallback) {
    auto found = effect.parameters.find(name);
    return found == effect.parameters.end() ? fallback : found->second;
}

void blend(AudioBuffer& audio, const std::vector<float>& dry, double wet) {
    wet = std::clamp(wet, 0.0, 1.0);
    for (size_t i = 0; i < audio.samples.size(); ++i)
        audio.samples[i] = static_cast<float>(dry[i] * (1.0 - wet) + audio.samples[i] * wet);
}

} // namespace

void processEffect(AudioBuffer& audio, const Effect& effect, int sampleRate) {
    if (audio.samples.empty() || audio.channels < 1) return;
    const std::vector<float> dry = audio.samples;
    const size_t frames = audio.samples.size() / static_cast<size_t>(audio.channels);

    if (effect.type == EffectType::LowPass || effect.type == EffectType::HighPass) {
        const double cutoff = std::clamp(parameter(effect, "cutoff", 1200.0), 10.0, sampleRate * 0.45);
        const double alpha = 1.0 - std::exp(-2.0 * PI * cutoff / sampleRate);
        std::vector<double> state(static_cast<size_t>(audio.channels), 0.0);
        for (size_t frame = 0; frame < frames; ++frame) {
            for (int channel = 0; channel < audio.channels; ++channel) {
                const size_t index = frame * audio.channels + channel;
                state[channel] += alpha * (audio.samples[index] - state[channel]);
                audio.samples[index] = static_cast<float>(effect.type == EffectType::LowPass
                    ? state[channel] : audio.samples[index] - state[channel]);
            }
        }
    } else if (effect.type == EffectType::ParametricEq) {
        const double frequency = std::clamp(parameter(effect, "frequency",
            parameter(effect, "freq", 1000.0)), 20.0, sampleRate * 0.45);
        const double gainDb = std::clamp(parameter(effect, "gain", 3.0), -24.0, 24.0);
        const double q = std::clamp(parameter(effect, "q", 1.0), 0.1, 20.0);
        const double a = std::pow(10.0, gainDb / 40.0);
        const double omega = 2.0 * PI * frequency / sampleRate;
        const double alpha = std::sin(omega) / (2.0 * q);
        const double a0 = 1.0 + alpha / a;
        const double b0 = (1.0 + alpha * a) / a0;
        const double b1 = (-2.0 * std::cos(omega)) / a0;
        const double b2 = (1.0 - alpha * a) / a0;
        const double a1 = (-2.0 * std::cos(omega)) / a0;
        const double a2 = (1.0 - alpha / a) / a0;
        std::vector<double> x1(audio.channels), x2(audio.channels), y1(audio.channels), y2(audio.channels);
        for (size_t frame = 0; frame < frames; ++frame) {
            for (int channel = 0; channel < audio.channels; ++channel) {
                const size_t index = frame * audio.channels + channel;
                const double input = audio.samples[index];
                const double output = b0 * input + b1 * x1[channel] + b2 * x2[channel]
                                    - a1 * y1[channel] - a2 * y2[channel];
                x2[channel] = x1[channel]; x1[channel] = input;
                y2[channel] = y1[channel]; y1[channel] = output;
                audio.samples[index] = static_cast<float>(output);
            }
        }
    } else if (effect.type == EffectType::Distortion || effect.type == EffectType::Saturation) {
        const double drive = std::max(1.0, parameter(effect, "drive", 3.0));
        for (float& sample : audio.samples) {
            const double shaped = effect.type == EffectType::Distortion
                ? std::tanh(sample * drive) / std::tanh(drive)
                : std::atan(sample * drive) / std::atan(drive);
            sample = static_cast<float>(shaped);
        }
    } else if (effect.type == EffectType::Bitcrusher) {
        const int bits = static_cast<int>(std::clamp(parameter(effect, "bits", 8.0), 2.0, 24.0));
        const double levels = std::pow(2.0, bits - 1) - 1.0;
        const int targetRate = static_cast<int>(std::clamp(parameter(effect, "rate", 11025.0), 1000.0,
                                                            static_cast<double>(sampleRate)));
        const size_t hold = std::max<size_t>(1, static_cast<size_t>(sampleRate / targetRate));
        for (int channel = 0; channel < audio.channels; ++channel) {
            for (size_t frame = 0; frame < frames; frame += hold) {
                const float value = static_cast<float>(std::round(audio.samples[frame * audio.channels + channel] * levels) / levels);
                for (size_t offset = 0; offset < hold && frame + offset < frames; ++offset)
                    audio.samples[(frame + offset) * audio.channels + channel] = value;
            }
        }
    } else if (effect.type == EffectType::Flanger) {
        const double rate = std::clamp(parameter(effect, "rate", 0.25), 0.01, 20.0);
        const double delayMs = std::clamp(parameter(effect, "delay", 2.0), 0.1, 15.0);
        const double depthMs = std::clamp(parameter(effect, "depth", 2.0), 0.0, 15.0);
        const double feedback = std::clamp(parameter(effect, "feedback", 0.45), -0.95, 0.95);
        for (size_t frame = 0; frame < frames; ++frame) {
            const double lfo = 0.5 + 0.5 * std::sin(2.0 * PI * rate * frame / sampleRate);
            const size_t offset = std::max<size_t>(1, static_cast<size_t>((delayMs + depthMs * lfo) * sampleRate / 1000.0));
            if (frame < offset) continue;
            for (int channel = 0; channel < audio.channels; ++channel) {
                const size_t index = frame * audio.channels + channel;
                const size_t delayed = (frame - offset) * audio.channels + channel;
                audio.samples[index] += static_cast<float>(audio.samples[delayed] * feedback);
            }
        }
    } else if (effect.type == EffectType::Phaser) {
        const double rate = std::clamp(parameter(effect, "rate", 0.35), 0.01, 20.0);
        const double depth = std::clamp(parameter(effect, "depth", 0.7), 0.0, 1.0);
        const int stages = static_cast<int>(std::clamp(parameter(effect, "stages", 4.0), 2.0, 12.0));
        std::vector<std::vector<double>> previousInput(stages, std::vector<double>(audio.channels));
        std::vector<std::vector<double>> previousOutput(stages, std::vector<double>(audio.channels));
        for (size_t frame = 0; frame < frames; ++frame) {
            const double sweep = 0.5 + 0.5 * std::sin(2.0 * PI * rate * frame / sampleRate);
            const double coefficient = std::clamp(0.05 + depth * sweep * 0.85, 0.01, 0.95);
            for (int channel = 0; channel < audio.channels; ++channel) {
                const size_t index = frame * audio.channels + channel;
                double value = audio.samples[index];
                for (int stage = 0; stage < stages; ++stage) {
                    const double output = -coefficient * value + previousInput[stage][channel]
                                        + coefficient * previousOutput[stage][channel];
                    previousInput[stage][channel] = value;
                    previousOutput[stage][channel] = output;
                    value = output;
                }
                audio.samples[index] = static_cast<float>(value);
            }
        }
    } else if (effect.type == EffectType::Delay || effect.type == EffectType::Reverb || effect.type == EffectType::Chorus) {
        const double milliseconds = effect.type == EffectType::Delay ? parameter(effect, "time", 250.0)
            : effect.type == EffectType::Chorus ? parameter(effect, "time", 18.0) : parameter(effect, "time", 83.0);
        const size_t delayFrames = std::max<size_t>(1, static_cast<size_t>(milliseconds * sampleRate / 1000.0));
        const double feedback = std::clamp(parameter(effect, "feedback",
            effect.type == EffectType::Reverb ? 0.45 : 0.3), 0.0, 0.95);
        for (size_t frame = delayFrames; frame < frames; ++frame)
            for (int channel = 0; channel < audio.channels; ++channel) {
                const size_t index = frame * audio.channels + channel;
                const size_t delayed = (frame - delayFrames) * audio.channels + channel;
                audio.samples[index] += static_cast<float>(audio.samples[delayed] * feedback);
            }
    } else if (effect.type == EffectType::Compressor || effect.type == EffectType::Limiter ||
               effect.type == EffectType::Gate || effect.type == EffectType::Expander) {
        const double thresholdDb = parameter(effect, "threshold", effect.type == EffectType::Limiter ? -1.0 : -18.0);
        const double threshold = std::pow(10.0, thresholdDb / 20.0);
        const double ratio = effect.type == EffectType::Limiter ? 100.0 : std::max(1.0, parameter(effect, "ratio", 4.0));
        for (float& sample : audio.samples) {
            const double magnitude = std::fabs(sample);
            if (effect.type == EffectType::Gate) {
                if (magnitude < threshold) sample = 0.0f;
            } else if (effect.type == EffectType::Expander && magnitude < threshold && magnitude > 1e-9) {
                const double gain = std::pow(magnitude / threshold, ratio - 1.0);
                sample = static_cast<float>(sample * gain);
            } else if (effect.type != EffectType::Expander && magnitude > threshold) {
                const double compressed = threshold + (magnitude - threshold) / ratio;
                sample = static_cast<float>(std::copysign(compressed, sample));
            }
        }
    } else if (effect.type == EffectType::DeEsser) {
        const double frequency = std::clamp(parameter(effect, "frequency", 6500.0), 1000.0, sampleRate * 0.45);
        const double threshold = std::pow(10.0, parameter(effect, "threshold", -24.0) / 20.0);
        const double ratio = std::max(1.0, parameter(effect, "ratio", 6.0));
        const double alpha = 1.0 - std::exp(-2.0 * PI * frequency / sampleRate);
        std::vector<double> low(audio.channels, 0.0);
        for (size_t frame = 0; frame < frames; ++frame) {
            for (int channel = 0; channel < audio.channels; ++channel) {
                const size_t index = frame * audio.channels + channel;
                const double input = audio.samples[index];
                low[channel] += alpha * (input - low[channel]);
                const double high = input - low[channel];
                const double magnitude = std::fabs(high);
                const double reduced = magnitude > threshold
                    ? threshold + (magnitude - threshold) / ratio : magnitude;
                audio.samples[index] = static_cast<float>(low[channel] + std::copysign(reduced, high));
            }
        }
    } else if (effect.type == EffectType::StereoWidth && audio.channels == 2) {
        const double width = std::clamp(parameter(effect, "width", 1.25), 0.0, 2.0);
        for (size_t frame = 0; frame < frames; ++frame) {
            const double left = audio.samples[frame * 2];
            const double right = audio.samples[frame * 2 + 1];
            const double mid = (left + right) * 0.5;
            const double side = (left - right) * 0.5 * width;
            audio.samples[frame * 2] = static_cast<float>(mid + side);
            audio.samples[frame * 2 + 1] = static_cast<float>(mid - side);
        }
    } else if (effect.type == EffectType::AutoPan && audio.channels == 2) {
        const double rate = std::max(0.01, parameter(effect, "rate", 0.5));
        const double depth = std::clamp(parameter(effect, "depth", 1.0), 0.0, 1.0);
        for (size_t frame = 0; frame < frames; ++frame) {
            const double phase = 2.0 * PI * rate * frame / sampleRate;
            const double pan = std::sin(phase) * depth;
            audio.samples[frame * 2] *= static_cast<float>(std::sqrt((1.0 - pan) * 0.5));
            audio.samples[frame * 2 + 1] *= static_cast<float>(std::sqrt((1.0 + pan) * 0.5));
        }
    }
    blend(audio, dry, effect.wet);
}

void processEffectChain(AudioBuffer& audio, const std::vector<Effect>& effects, int sampleRate) {
    for (const auto& effect : effects) processEffect(audio, effect, sampleRate);
}

void processSidechain(AudioBuffer& target, const AudioBuffer& key,
                      const Sidechain& settings, int sampleRate) {
    if (target.samples.empty() || key.samples.empty() || target.channels < 1 || key.channels < 1) return;
    const size_t frames = std::min(target.samples.size() / static_cast<size_t>(target.channels),
                                   key.samples.size() / static_cast<size_t>(key.channels));
    const double threshold = std::pow(10.0, settings.thresholdDb / 20.0);
    const double attack = std::exp(-1.0 / (settings.attackMs * 0.001 * sampleRate));
    const double release = std::exp(-1.0 / (settings.releaseMs * 0.001 * sampleRate));
    double envelope = 0.0;
    for (size_t frame = 0; frame < frames; ++frame) {
        double keyLevel = 0.0;
        for (int channel = 0; channel < key.channels; ++channel)
            keyLevel = std::max(keyLevel, std::fabs(static_cast<double>(key.samples[frame * key.channels + channel])));
        const double coefficient = keyLevel > envelope ? attack : release;
        envelope = coefficient * envelope + (1.0 - coefficient) * keyLevel;
        double compressedGain = 1.0;
        if (envelope > threshold && threshold > 0.0) {
            const double over = envelope / threshold;
            compressedGain = std::pow(over, -(1.0 - 1.0 / settings.ratio));
        }
        const double gain = 1.0 + (compressedGain - 1.0) * settings.amount;
        for (int channel = 0; channel < target.channels; ++channel)
            target.samples[frame * target.channels + channel] *= static_cast<float>(gain);
    }
}

} // namespace lyra
