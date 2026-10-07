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
    } else if (effect.type == EffectType::Compressor || effect.type == EffectType::Limiter || effect.type == EffectType::Gate) {
        const double thresholdDb = parameter(effect, "threshold", effect.type == EffectType::Limiter ? -1.0 : -18.0);
        const double threshold = std::pow(10.0, thresholdDb / 20.0);
        const double ratio = effect.type == EffectType::Limiter ? 100.0 : std::max(1.0, parameter(effect, "ratio", 4.0));
        for (float& sample : audio.samples) {
            const double magnitude = std::fabs(sample);
            if (effect.type == EffectType::Gate) {
                if (magnitude < threshold) sample = 0.0f;
            } else if (magnitude > threshold) {
                const double compressed = threshold + (magnitude - threshold) / ratio;
                sample = static_cast<float>(std::copysign(compressed, sample));
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

} // namespace lyra
