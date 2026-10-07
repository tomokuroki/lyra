#include "synth.hpp"
#include "dsp.hpp"
#include "audio_file.hpp"
#include <cmath>

namespace lyra {

static double sampleWave(WaveType wave, double phase, double t) {
    auto noiseAt = [](int64_t index) {
        uint32_t x = static_cast<uint32_t>(index) * 747796405u + 2891336453u;
        x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
        x = (x >> 22u) ^ x;
        return static_cast<int32_t>(x) / 2147483648.0;
    };
    switch (wave) {
        case WaveType::Sine:     return std::sin(2.0 * PI * phase);
        case WaveType::Square:   return phase < 0.5 ? 1.0 : -1.0;
        case WaveType::Triangle: return 4.0 * std::fabs(phase - 0.5) - 1.0;
        case WaveType::Saw:      return 2.0 * phase - 1.0;
        case WaveType::Pulse:    return phase < 0.25 ? 1.0 : -1.0;
        case WaveType::Noise: {
            return noiseAt(static_cast<int64_t>(t * 44100.0 * 7.0 + phase * 1e6));
        }
        case WaveType::PinkNoise: {
            const int64_t index = static_cast<int64_t>(t * 44100.0);
            return 0.52 * noiseAt(index) + 0.26 * noiseAt(index / 2)
                 + 0.14 * noiseAt(index / 4) + 0.08 * noiseAt(index / 8);
        }
        case WaveType::BrownNoise: {
            const double position = t * 180.0;
            const int64_t index = static_cast<int64_t>(std::floor(position));
            const double fraction = position - index;
            return noiseAt(index) * (1.0 - fraction) + noiseAt(index + 1) * fraction;
        }
        case WaveType::BlueNoise: {
            const int64_t index = static_cast<int64_t>(t * 44100.0);
            return std::clamp((noiseAt(index) - noiseAt(index - 1)) * 0.7, -1.0, 1.0);
        }
    }
    return 0.0;
}

static double lfoValue(const LfoRoute& route, double t) {
    double phase = std::fmod(std::max(0.0, t * route.rateHz), 1.0);
    switch (route.wave) {
        case LfoWave::Sine: return std::sin(2.0 * PI * phase);
        case LfoWave::Triangle: return 1.0 - 4.0 * std::fabs(phase - 0.5);
        case LfoWave::Random: {
            uint32_t x = static_cast<uint32_t>(std::floor(t * route.rateHz + 1.0)) * 747796405u + 2891336453u;
            x ^= x >> 16; x *= 2246822519u; x ^= x >> 13;
            return static_cast<int32_t>(x) / 2147483648.0;
        }
    }
    return 0.0;
}

static double instrumentSample(const NoteEvent& ev, double phase, double t, double freq,
                               SoundMode soundMode) {
    auto sine = [&](double multiple, double offset = 0.0) {
        return std::sin(2.0 * PI * (phase * multiple + offset));
    };

    if (soundMode == SoundMode::FourBit) {
        double duty = ev.instrument == InstrumentType::SynthBass ? 0.5 : 0.25;
        return phase < duty ? 0.82 : -0.82;
    }
    if (soundMode == SoundMode::FM) {
        double ratio = 2.0 + (static_cast<int>(ev.instrument) % 4) * 0.5;
        double index = ev.instrument == InstrumentType::SynthBass ? 1.2 : 2.8;
        double modulator = std::sin(2.0 * PI * phase * ratio) * index;
        return 0.82 * std::sin(2.0 * PI * phase + modulator)
             + 0.18 * std::sin(2.0 * PI * phase * 2.0);
    }

    switch (ev.instrument) {
        case InstrumentType::Wave:
            return sampleWave(ev.wave, phase, t);
        case InstrumentType::Piano:
            return 0.62 * sine(1.0) + 0.24 * sine(2.0) * std::exp(-t * 1.8)
                 + 0.10 * sine(3.0) * std::exp(-t * 2.8)
                 + 0.04 * sampleWave(WaveType::Noise, phase, t) * std::exp(-t * 45.0);
        case InstrumentType::ElectricPiano:
            return 0.72 * sine(1.0) + 0.18 * sine(2.0)
                 + 0.10 * std::sin(2.0 * PI * phase + 2.2 * std::sin(2.0 * PI * phase * 2.0));
        case InstrumentType::Organ:
            return 0.58 * sine(1.0) + 0.24 * sine(2.0) + 0.12 * sine(3.0) + 0.06 * sine(4.0);
        case InstrumentType::MusicBox:
            return 0.56 * sine(1.0) + 0.28 * sine(3.0) + 0.16 * sine(5.0);
        case InstrumentType::Glockenspiel:
            return 0.48 * sine(1.0) + 0.30 * sine(2.76) + 0.15 * sine(5.4) + 0.07 * sine(8.9);
        case InstrumentType::Strings:
            return 0.56 * sampleWave(WaveType::Saw, phase, t) + 0.22 * sine(1.003)
                 + 0.22 * sine(0.997);
        case InstrumentType::Brass:
            return std::tanh(1.8 * (0.68 * sampleWave(WaveType::Saw, phase, t) + 0.32 * sine(1.0)));
        case InstrumentType::Flute:
            return 0.88 * sine(1.0) + 0.08 * sine(2.0) + 0.04 * sampleWave(WaveType::Noise, phase, t);
        case InstrumentType::Guitar:
            return 0.58 * sine(1.0) + 0.25 * sine(2.0) + 0.12 * sine(3.0) + 0.05 * sine(4.0);
        case InstrumentType::ElectricGuitar: {
            double raw = 0.65 * sampleWave(WaveType::Saw, phase, t) + 0.35 * sine(1.0);
            return std::tanh(raw * 3.2);
        }
        case InstrumentType::SynthBass:
            return 0.58 * sampleWave(WaveType::Square, phase, t) + 0.32 * sine(1.0)
                 + 0.10 * sine(freq < 80.0 ? 2.0 : 0.5);
    }
    return 0.0;
}

static double instrumentEnvelope(const NoteEvent& ev, double t, double duration) {
    double attack = 0.008;
    double release = 0.05;
    double sustain = 1.0;
    switch (ev.instrument) {
        case InstrumentType::Piano:         attack = 0.004; release = 0.14; sustain = 0.22 + 0.78 * std::exp(-t * 1.25); break;
        case InstrumentType::ElectricPiano: attack = 0.008; release = 0.16; sustain = 0.35 + 0.65 * std::exp(-t * 0.8); break;
        case InstrumentType::Organ:         attack = 0.015; release = 0.10; break;
        case InstrumentType::MusicBox:      attack = 0.002; release = 0.12; sustain = std::exp(-t * 2.8); break;
        case InstrumentType::Glockenspiel:  attack = 0.002; release = 0.18; sustain = std::exp(-t * 2.0); break;
        case InstrumentType::Strings:       attack = 0.10;  release = 0.20; break;
        case InstrumentType::Brass:         attack = 0.035; release = 0.12; break;
        case InstrumentType::Flute:         attack = 0.045; release = 0.13; break;
        case InstrumentType::Guitar:        attack = 0.003; release = 0.10; sustain = 0.18 + 0.82 * std::exp(-t * 1.7); break;
        case InstrumentType::ElectricGuitar:attack = 0.006; release = 0.12; sustain = 0.55 + 0.45 * std::exp(-t * 0.7); break;
        case InstrumentType::SynthBass:     attack = 0.004; release = 0.07; sustain = 0.70 + 0.30 * std::exp(-t * 1.0); break;
        case InstrumentType::Wave: break;
    }
    if (ev.attack >= 0.0) attack = ev.attack;
    if (ev.release >= 0.0) release = ev.release;
    if (ev.decay >= 0.0 && ev.sustain >= 0.0) {
        const double sustainLevel = std::clamp(ev.sustain, 0.0, 1.0);
        double env;
        if (attack > 0.0 && t < attack) env = t / attack;
        else if (ev.decay > 0.0 && t < attack + ev.decay) {
            const double position = (t - attack) / ev.decay;
            env = 1.0 + (sustainLevel - 1.0) * std::clamp(position, 0.0, 1.0);
        } else env = sustainLevel;
        if (release > 0.0 && t > duration - release)
            env *= std::max(0.0, (duration - t) / release);
        return env;
    }
    double env = sustain;
    if (t < attack) env *= t / attack;
    if (t > duration - release) env *= std::max(0.0, (duration - t) / release);
    return env;
}

static void renderDrum(const NoteEvent& ev, size_t startS, size_t nS,
                       std::vector<double>& mixL, std::vector<double>& mixR, int sampleRate) {
    double pitch = 1.0;
    double decay = 1.0;
    double color = 1.0;
    switch (ev.drumKit) {
        case DrumKit::Standard: break;
        case DrumKit::Rock:       pitch = 0.88; decay = 0.78; color = 1.18; break;
        case DrumKit::Electronic: pitch = 1.18; decay = 1.18; color = 0.92; break;
        case DrumKit::Retro:      pitch = 1.35; decay = 1.45; color = 0.70; break;
        case DrumKit::Orchestral: pitch = 0.72; decay = 0.55; color = 1.05; break;
    }
    double panAngle = (ev.pan + 1.0) * PI * 0.25;
    double gainL = std::cos(panAngle);
    double gainR = std::sin(panAngle);
    double filterState = 0.0;
    double filterAlpha = ev.cutoff > 0.0
        ? std::min(1.0, 1.0 - std::exp(-2.0 * PI * ev.cutoff / sampleRate)) : 1.0;
    for (size_t i = 0; i < nS && startS + i < mixL.size(); ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double env = 1.0;
        double sample = 0.0;

        switch (ev.drum) {
            case DrumType::Kick: {
                double freq = 155.0 * pitch * std::exp(-t * 25.0 * decay);
                env = std::exp(-t * 12.0 * decay);
                sample = std::sin(2.0 * PI * freq * t) * 0.9
                       + sampleWave(WaveType::Noise, t * 3.0, t) * 0.3;
                break;
            }
            case DrumType::Snare: {
                env = std::exp(-t * 18.0 * decay);
                sample = sampleWave(WaveType::Noise, t * 8.0, t) * 0.78 * color
                       + std::sin(2.0 * PI * 205.0 * pitch * t) * 0.30 * std::exp(-t * 30.0);
                break;
            }
            case DrumType::Hihat: {
                env = std::exp(-t * 48.0 * decay);
                sample = sampleWave(WaveType::Noise, t * 20.0, t) * 0.52 * color;
                break;
            }
            case DrumType::OpenHihat:
                env = std::exp(-t * 8.0 * decay);
                sample = sampleWave(WaveType::Noise, t * 25.0, t) * 0.48 * color;
                break;
            case DrumType::TomLow:
            case DrumType::TomMid:
            case DrumType::TomHigh: {
                double base = ev.drum == DrumType::TomLow ? 82.0 : (ev.drum == DrumType::TomHigh ? 185.0 : 125.0);
                double freq = base * pitch * std::exp(-t * 7.0);
                env = std::exp(-t * 8.0 * decay);
                sample = std::sin(2.0 * PI * freq * t);
                break;
            }
            case DrumType::Clap: {
                double bursts = (t < 0.018 || (t > 0.028 && t < 0.046) || (t > 0.057 && t < 0.078)) ? 1.0 : 0.32;
                env = std::exp(-t * 14.0 * decay) * bursts;
                sample = sampleWave(WaveType::Noise, t * 31.0, t) * 0.72 * color;
                break;
            }
            case DrumType::Rimshot:
                env = std::exp(-t * 55.0 * decay);
                sample = 0.65 * std::sin(2.0 * PI * 1650.0 * pitch * t)
                       + 0.35 * sampleWave(WaveType::Noise, t * 13.0, t);
                break;
            case DrumType::Crash:
                env = std::exp(-t * 2.8 * decay);
                sample = sampleWave(WaveType::Noise, t * 47.0, t) * 0.48 * color
                       + 0.10 * std::sin(2.0 * PI * 5300.0 * t);
                break;
            case DrumType::Ride:
                env = std::exp(-t * 4.2 * decay);
                sample = 0.33 * sampleWave(WaveType::Noise, t * 41.0, t)
                       + 0.22 * std::sin(2.0 * PI * 2400.0 * pitch * t)
                       + 0.16 * std::sin(2.0 * PI * 3900.0 * pitch * t);
                break;
            case DrumType::Cowbell:
                env = std::exp(-t * 9.0 * decay);
                sample = 0.55 * sampleWave(WaveType::Square, std::fmod(540.0 * pitch * t, 1.0), t)
                       + 0.35 * sampleWave(WaveType::Square, std::fmod(800.0 * pitch * t, 1.0), t);
                break;
            case DrumType::Shaker:
                env = std::exp(-t * 24.0 * decay);
                sample = sampleWave(WaveType::Noise, t * 61.0, t) * 0.38 * color;
                break;
            case DrumType::Tambourine:
                env = std::exp(-t * 11.0 * decay);
                sample = sampleWave(WaveType::Noise, t * 53.0, t) * 0.42 * color
                       + 0.12 * std::sin(2.0 * PI * 6100.0 * t);
                break;
            case DrumType::Timpani: {
                double freq = 92.0 * pitch * std::exp(-t * 2.5);
                env = std::exp(-t * 3.8 * decay);
                sample = 0.84 * std::sin(2.0 * PI * freq * t) + 0.16 * std::sin(4.0 * PI * freq * t);
                break;
            }
            case DrumType::Impact:
                env = std::exp(-t * 3.0 * decay);
                sample = 0.56 * sampleWave(WaveType::Noise, t * 5.0, t)
                       + 0.64 * std::sin(2.0 * PI * (70.0 * pitch * std::exp(-t * 5.0)) * t);
                break;
            default: break;
        }
        if (ev.drumKit == DrumKit::Retro)
            sample = std::round(sample * 7.0) / 7.0;
        if (ev.drive > 0.0) {
            double amount = 1.0 + ev.drive * 5.0;
            sample = std::tanh(sample * amount) / std::tanh(amount);
        }
        filterState += filterAlpha * (sample - filterState);
        sample = filterState;
        double value = sample * env * ev.volume;
        mixL[startS + i] += value * gainL;
        mixR[startS + i] += value * gainR;
    }
}

static AudioBuffer generateSamplesImpl(const std::vector<NoteEvent>& events, const Config& cfg,
                                       const Project* project, bool raw = false,
                                       size_t forcedFrames = 0) {
    if (events.empty() && forcedFrames == 0) return AudioBuffer{{}, cfg.channels};

    double maxBeat = 0.0;
    for (const auto& e : events)
        maxBeat = std::max(maxBeat, e.startBeat + e.durationBeats);

    double beatSec = 60.0 / cfg.tempo;
    double effectTail = (cfg.reverb > 0.0 ? 1.4 : 0.0)
                      + (cfg.delayMix > 0.0 ? cfg.delayBeats * beatSec * 2.0 : 0.0);
    double totalSec = (project ? project->beatToSeconds(maxBeat) : maxBeat * beatSec) + effectTail;
    size_t totalSamples = forcedFrames > 0 ? forcedFrames
        : static_cast<size_t>(totalSec * cfg.sampleRate + 0.5);
    std::vector<double> mixL(totalSamples, 0.0);
    std::vector<double> mixR(totalSamples, 0.0);

    for (const auto& ev : events) {
        const double startSec = project ? project->beatToSeconds(ev.startBeat) : ev.startBeat * beatSec;
        const double durationSec = project ? project->durationSeconds(ev.startBeat, ev.durationBeats)
                                           : ev.durationBeats * beatSec;
        size_t startS = static_cast<size_t>(startSec * cfg.sampleRate + 0.5);
        size_t nS = static_cast<size_t>(durationSec * cfg.sampleRate + 0.5);
        if (nS == 0) continue;

        if (ev.drum != DrumType::None) {
            renderDrum(ev, startS, nS, mixL, mixR, cfg.sampleRate);
            continue;
        }

        size_t notes = std::min(ev.freqs.size(), size_t(16));
        if (notes == 0) continue;

        double durSec = durationSec;
        double filterLow = 0.0;
        double filterBand = 0.0;

        for (size_t i = 0; i < nS && startS + i < totalSamples; ++i) {
            double t = static_cast<double>(i) / cfg.sampleRate;
            double env = instrumentEnvelope(ev, t, durSec);
            const double progress = durSec > 0.0 ? std::clamp(t / durSec, 0.0, 1.0) : 0.0;
            double pitchMod = ev.pitchEnvelopeStart
                + (ev.pitchEnvelopeEnd - ev.pitchEnvelopeStart) * progress;
            double cutoff = ev.cutoff;
            if (ev.filterEnvelopeStart > 0.0 || ev.filterEnvelopeEnd > 0.0)
                cutoff = ev.filterEnvelopeStart + (ev.filterEnvelopeEnd - ev.filterEnvelopeStart) * progress;
            double pan = ev.pan;
            double amplitudeMod = 1.0;
            for (const auto& route : ev.lfoRoutes) {
                const double value = lfoValue(route, t) * route.amount;
                if (route.target == ModTarget::Pitch) pitchMod += value;
                else if (route.target == ModTarget::Cutoff) cutoff += value;
                else if (route.target == ModTarget::Pan) pan += value;
                else if (route.target == ModTarget::Amp) amplitudeMod *= std::max(0.0, 1.0 + value);
            }

            double sample = 0.0;
            const size_t oscillatorCount = ev.oscillators.empty() ? 1 : ev.oscillators.size();
            double oscillatorLevels = ev.oscillators.empty() ? 1.0 : 0.0;
            for (const auto& oscillator : ev.oscillators) oscillatorLevels += oscillator.level;
            oscillatorLevels = std::max(oscillatorLevels, 1e-9);
            for (size_t noteIndex = 0; noteIndex < notes; ++noteIndex) {
                const double baseFrequency = ev.freqs[noteIndex];
                if (baseFrequency <= 0.0) continue;
                for (size_t oscillatorIndex = 0; oscillatorIndex < oscillatorCount; ++oscillatorIndex) {
                    NoteEvent voiceEvent = ev;
                    double level = 1.0;
                    double offsetCents = 0.0;
                    if (!ev.oscillators.empty()) {
                        const auto& oscillator = ev.oscillators[oscillatorIndex];
                        voiceEvent.wave = oscillator.wave;
                        level = oscillator.level;
                        offsetCents = oscillator.detuneCents + oscillator.semitones * 100.0;
                    }
                    const int unison = std::max(1, ev.unisonVoices);
                    for (int unisonIndex = 0; unisonIndex < unison; ++unisonIndex) {
                        const double spread = unison == 1 ? 0.0
                            : (2.0 * unisonIndex / static_cast<double>(unison - 1) - 1.0) * ev.unisonDetuneCents;
                        const double frequency = baseFrequency * std::pow(2.0,
                            (offsetCents + spread + pitchMod * 100.0) / 1200.0);
                        double phase = frequency * t;
                        if (ev.fmRatio > 0.0 && ev.fmAmount > 0.0)
                            phase += std::sin(2.0 * PI * frequency * ev.fmRatio * t) * ev.fmAmount / (2.0 * PI);
                        phase -= std::floor(phase);
                        sample += instrumentSample(voiceEvent, phase, t, frequency, cfg.soundMode)
                            * level / (notes * oscillatorLevels * unison);
                    }
                }
            }
            if (ev.amRate > 0.0 && ev.amDepth > 0.0)
                sample *= (1.0 - ev.amDepth) + ev.amDepth * (0.5 + 0.5 * std::sin(2.0 * PI * ev.amRate * t));
            if (ev.drive > 0.0) {
                double amount = 1.0 + ev.drive * 5.0;
                sample = std::tanh(sample * amount) / std::tanh(amount);
            }
            if (cutoff > 0.0) {
                const double coefficient = std::clamp(2.0 * std::sin(PI * std::min(cutoff, cfg.sampleRate * 0.45)
                    / cfg.sampleRate), 0.0, 0.99);
                const double damping = 1.5 - std::clamp(ev.resonance, 0.0, 1.0) * 1.4;
                filterLow += coefficient * filterBand;
                const double high = sample - filterLow - damping * filterBand;
                filterBand += coefficient * high;
                if (ev.filterType == FilterType::LowPass) sample = filterLow;
                else if (ev.filterType == FilterType::HighPass) sample = high;
                else if (ev.filterType == FilterType::BandPass) sample = filterBand;
                else sample = filterLow + high;
            }
            double panAngle = (std::clamp(pan, -1.0, 1.0) + 1.0) * PI * 0.25;
            double value = sample * env * ev.volume * amplitudeMod;
            mixL[startS + i] += value * std::cos(panAngle);
            mixR[startS + i] += value * std::sin(panAngle);
        }
    }

    if (raw) {
        AudioBuffer out;
        out.channels = cfg.channels;
        out.samples.resize(totalSamples * static_cast<size_t>(out.channels));
        for (size_t i = 0; i < totalSamples; ++i) {
            if (out.channels == 1) out.samples[i] = static_cast<float>((mixL[i] + mixR[i]) * 0.5);
            else {
                out.samples[i * 2] = static_cast<float>(mixL[i]);
                out.samples[i * 2 + 1] = static_cast<float>(mixR[i]);
            }
        }
        return out;
    }

    auto processChannel = [&](std::vector<double>& mix) {
    // Musical echo with two decaying repeats.
    if (cfg.delayMix > 0.0 && cfg.delayBeats > 0.0) {
        size_t delay = static_cast<size_t>(cfg.delayBeats * beatSec * cfg.sampleRate);
        if (delay > 0) {
            std::vector<double> dry = mix;
            for (size_t i = delay; i < mix.size(); ++i) {
                mix[i] += dry[i - delay] * cfg.delayMix;
                if (i >= delay * 2) mix[i] += dry[i - delay * 2] * cfg.delayMix * 0.38;
            }
        }
    }

    // A small room made from several short reflections. Keeping it simple and
    // deterministic makes the same .lyra file render identically everywhere.
    if (cfg.reverb > 0.0) {
        std::vector<double> dry = mix;
        const double taps[] = {0.037, 0.061, 0.089, 0.127, 0.181};
        const double gains[] = {0.34, 0.27, 0.21, 0.16, 0.11};
        for (size_t tap = 0; tap < 5; ++tap) {
            size_t offset = static_cast<size_t>(taps[tap] * cfg.sampleRate);
            for (size_t i = offset; i < mix.size(); ++i)
                mix[i] += dry[i - offset] * gains[tap] * cfg.reverb;
        }
    }

    auto holdAndQuantize = [&](int targetRate, double levels) {
        size_t hold = std::max<size_t>(1, static_cast<size_t>(cfg.sampleRate / targetRate));
        for (size_t i = 0; i < mix.size(); i += hold) {
            double crushed = std::round(mix[i] * levels) / levels;
            for (size_t j = 0; j < hold && i + j < mix.size(); ++j) mix[i + j] = crushed;
        }
    };

    if (cfg.soundMode == SoundMode::FourBit) {
        // Toy-like early chips: roughly 6 kHz playback and only 15 amplitude values.
        holdAndQuantize(6000, 7.0);
    } else if (cfg.soundMode == SoundMode::EightBit) {
        // Classic console character: lower effective sample rate and coarse amplitude steps.
        holdAndQuantize(11025, 31.0);
    } else if (cfg.soundMode == SoundMode::SixteenBit) {
        // A warm 16-bit-console color: subtle chorus, gentle low-pass, fine quantization.
        std::vector<double> dry = mix;
        double filtered = 0.0;
        for (size_t i = 0; i < mix.size(); ++i) {
            double time = static_cast<double>(i) / cfg.sampleRate;
            double delaySec = 0.012 + 0.003 * std::sin(2.0 * PI * 0.7 * time);
            size_t offset = static_cast<size_t>(delaySec * cfg.sampleRate);
            double chorus = i >= offset ? dry[i - offset] * 0.11 : 0.0;
            filtered += 0.62 * ((dry[i] + chorus) - filtered);
            mix[i] = std::round(filtered * 4095.0) / 4095.0;
        }
    } else if (cfg.soundMode == SoundMode::ThirtyTwoBit) {
        // Early sample-console color: an ADPCM-like predictor and a slightly dark output stage.
        double predictor = 0.0;
        double filtered = 0.0;
        for (double& sample : mix) {
            double delta = std::max(-0.09, std::min(0.09, sample - predictor));
            predictor = std::round((predictor + delta) * 2047.0) / 2047.0;
            filtered += 0.76 * (predictor - filtered);
            sample = filtered;
        }
    } else if (cfg.soundMode == SoundMode::SixtyFourBit) {
        // Later-console ambience: a clean signal with slow modulation and spacious reflections.
        std::vector<double> dry = mix;
        for (size_t i = 0; i < mix.size(); ++i) {
            double time = static_cast<double>(i) / cfg.sampleRate;
            size_t modDelay = static_cast<size_t>((0.018 + 0.006 * std::sin(2.0 * PI * 0.23 * time)) * cfg.sampleRate);
            double wide = i >= modDelay ? dry[i - modDelay] : 0.0;
            size_t roomDelay = static_cast<size_t>(0.143 * cfg.sampleRate);
            double room = i >= roomDelay ? dry[i - roomDelay] : 0.0;
            mix[i] = dry[i] + wide * 0.09 + room * 0.07;
        }
    } else if (cfg.soundMode == SoundMode::Tracker) {
        // Amiga/tracker modules used short, gritty samples with limited playback resolution.
        holdAndQuantize(22050, 127.0);
        std::vector<double> dry = mix;
        size_t slap = static_cast<size_t>(0.006 * cfg.sampleRate);
        for (size_t i = slap; i < mix.size(); ++i) mix[i] += dry[i - slap] * 0.08;
    } else if (cfg.soundMode == SoundMode::ChiptuneModern) {
        // Preserve a polished master while blending in a quiet crushed chip layer.
        std::vector<double> clean = mix;
        std::vector<double> chip = mix;
        size_t hold = std::max<size_t>(1, static_cast<size_t>(cfg.sampleRate / 11025));
        for (size_t i = 0; i < chip.size(); i += hold) {
            double crushed = std::round(chip[i] * 31.0) / 31.0;
            for (size_t j = 0; j < hold && i + j < chip.size(); ++j) chip[i + j] = crushed;
        }
        for (size_t i = 0; i < mix.size(); ++i)
            mix[i] = std::tanh(clean[i] * 1.18) * 0.90 + chip[i] * 0.16;
    }

    // Soft master saturation is friendlier than hard clipping when many tracks meet.
    for (double& sample : mix)
        sample = std::tanh(sample * 1.12) / std::tanh(1.12);
    };

    processChannel(mixL);
    processChannel(mixR);

    size_t fadeInSamples = static_cast<size_t>(cfg.fadeInBeats * beatSec * cfg.sampleRate);
    fadeInSamples = std::min(fadeInSamples, totalSamples);
    for (size_t i = 0; i < fadeInSamples; ++i) {
        double gain = static_cast<double>(i) / std::max<size_t>(1, fadeInSamples);
        mixL[i] *= gain;
        mixR[i] *= gain;
    }

    size_t fadeOutSamples = static_cast<size_t>(cfg.fadeOutBeats * beatSec * cfg.sampleRate);
    fadeOutSamples = std::min(fadeOutSamples, totalSamples);
    for (size_t i = 0; i < fadeOutSamples; ++i) {
        double gain = static_cast<double>(fadeOutSamples - i) / std::max<size_t>(1, fadeOutSamples);
        size_t pos = totalSamples - fadeOutSamples + i;
        mixL[pos] *= gain;
        mixR[pos] *= gain;
    }

    double peak = 0.0;
    for (double s : mixL) peak = std::max(peak, std::fabs(s));
    for (double s : mixR) peak = std::max(peak, std::fabs(s));
    double targetPeak = std::pow(10.0, cfg.masterPeakDb / 20.0);
    double norm = peak > 1e-12 ? targetPeak / peak : 1.0;

    AudioBuffer out;
    out.channels = cfg.channels;
    out.samples.resize(totalSamples * static_cast<size_t>(out.channels));
    for (size_t i = 0; i < totalSamples; ++i) {
        double left = std::max(-1.0, std::min(1.0, mixL[i] * norm));
        double right = std::max(-1.0, std::min(1.0, mixR[i] * norm));
        if (out.channels == 1) {
            out.samples[i] = static_cast<float>((left + right) * 0.5);
        } else {
            out.samples[i * 2] = static_cast<float>(left);
            out.samples[i * 2 + 1] = static_cast<float>(right);
        }
    }
    return out;
}

AudioBuffer generateSamples(const std::vector<NoteEvent>& events, const Config& cfg) {
    return generateSamplesImpl(events, cfg, nullptr);
}

static void renderAudioClip(AudioBuffer& target, const AudioClip& clip, const Project& project,
                            const MixerChannel& mixer) {
    const SourceAudio source = loadAudioFile(clip.path);
    if (source.samples.empty() || source.channels <= 0 || source.sampleRate <= 0) return;
    const size_t sourceFrames = source.samples.size() / static_cast<size_t>(source.channels);
    const size_t trimStart = std::min(sourceFrames, static_cast<size_t>(clip.trimStartSeconds * source.sampleRate));
    const size_t trimEnd = clip.trimEndSeconds < 0.0 ? sourceFrames
        : std::min(sourceFrames, static_cast<size_t>(clip.trimEndSeconds * source.sampleRate));
    if (trimEnd <= trimStart) return;
    const size_t trimmedFrames = trimEnd - trimStart;
    const size_t startFrame = static_cast<size_t>(project.beatToSeconds(clip.startBeat) * project.config.sampleRate + 0.5);
    const size_t outputFrames = static_cast<size_t>(project.durationSeconds(clip.startBeat, clip.lengthBeats)
                                                    * project.config.sampleRate + 0.5);
    const double pitchFactor = std::pow(2.0, clip.pitchSemitones / 12.0);
    const double step = trimmedFrames / static_cast<double>(std::max<size_t>(1, outputFrames))
                      * pitchFactor / clip.stretch;
    const size_t crossfadeFrames = static_cast<size_t>(clip.crossfadeMs * source.sampleRate / 1000.0);
    const size_t fadeInFrames = static_cast<size_t>(project.durationSeconds(clip.startBeat, clip.fadeInBeats)
                                                    * project.config.sampleRate + 0.5);
    const size_t fadeOutFrames = static_cast<size_t>(project.durationSeconds(
        clip.startBeat + std::max(0.0, clip.lengthBeats - clip.fadeOutBeats), clip.fadeOutBeats)
        * project.config.sampleRate + 0.5);
    const double panAngle = (std::clamp(mixer.pan, -1.0, 1.0) + 1.0) * PI * 0.25;
    for (size_t output = 0; output < outputFrames && startFrame + output < target.samples.size() / target.channels; ++output) {
        double position = output * step;
        if (clip.loop) position = std::fmod(position, static_cast<double>(trimmedFrames));
        else if (position >= trimmedFrames) break;
        if (clip.reverse) position = (trimmedFrames - 1) - position;
        const size_t leftIndex = static_cast<size_t>(std::floor(position));
        const size_t rightIndex = std::min(trimmedFrames - 1, leftIndex + 1);
        const double fraction = position - leftIndex;
        double gain = clip.gain * mixer.gain;
        if (fadeInFrames > 0 && output < fadeInFrames) gain *= output / static_cast<double>(fadeInFrames);
        if (fadeOutFrames > 0 && output + fadeOutFrames > outputFrames)
            gain *= (outputFrames - output) / static_cast<double>(fadeOutFrames);
        for (int channel = 0; channel < target.channels; ++channel) {
            const int sourceChannel = source.channels == 1 ? 0 : std::min(channel, source.channels - 1);
            double value = source.samples[(trimStart + leftIndex) * source.channels + sourceChannel] * (1.0 - fraction)
                         + source.samples[(trimStart + rightIndex) * source.channels + sourceChannel] * fraction;
            if (clip.loop && crossfadeFrames > 0 && !clip.reverse && leftIndex + crossfadeFrames >= trimmedFrames) {
                const double blend = (leftIndex + crossfadeFrames - trimmedFrames) / static_cast<double>(crossfadeFrames);
                const size_t wrap = (leftIndex + crossfadeFrames - trimmedFrames) % trimmedFrames;
                const double wrapped = source.samples[(trimStart + wrap) * source.channels + sourceChannel];
                value = value * (1.0 - blend) + wrapped * blend;
            }
            const double channelGain = target.channels == 2
                ? gain * (channel == 0 ? std::cos(panAngle) : std::sin(panAngle)) : gain;
            target.samples[(startFrame + output) * target.channels + channel] += static_cast<float>(value * channelGain);
        }
    }
}

AudioBuffer generateSamples(const Project& project) {
    const bool hasGraph = !project.buses.empty() || !project.master.inserts.empty() ||
        std::any_of(project.tracks.begin(), project.tracks.end(), [](const Track& track) {
            return !track.inserts.empty() || !track.sends.empty() || !track.sidechains.empty() || !track.clips.empty();
        });
    if (!hasGraph) return generateSamplesImpl(project.renderEvents(), project.config, &project);

    const auto flattened = project.renderEvents();
    const size_t frames = static_cast<size_t>((project.beatToSeconds(project.durationBeats()) + 3.0)
                                               * project.config.sampleRate + 0.5);
    AudioBuffer master{std::vector<float>(frames * static_cast<size_t>(project.config.channels), 0.0f),
                       project.config.channels};
    std::vector<AudioBuffer> busAudio(project.buses.size(),
        AudioBuffer{std::vector<float>(master.samples.size(), 0.0f), project.config.channels});
    std::vector<AudioBuffer> trackAudio(project.tracks.size(),
        AudioBuffer{std::vector<float>(master.samples.size(), 0.0f), project.config.channels});

    auto addScaled = [](AudioBuffer& target, const AudioBuffer& source, double gain) {
        const size_t count = std::min(target.samples.size(), source.samples.size());
        for (size_t i = 0; i < count; ++i)
            target.samples[i] += static_cast<float>(source.samples[i] * gain);
    };

    for (size_t trackIndex = 0; trackIndex < project.tracks.size(); ++trackIndex) {
        const auto& track = project.tracks[trackIndex];
        const bool hasSolo = std::any_of(project.tracks.begin(), project.tracks.end(),
            [](const Track& candidate) { return candidate.mixer.solo; });
        if (track.mixer.mute || (hasSolo && !track.mixer.solo)) continue;
        std::vector<NoteEvent> trackEvents;
        for (const auto& event : flattened)
            if (event.trackId == track.id) trackEvents.push_back(event);
        if (!trackEvents.empty())
            trackAudio[trackIndex] = generateSamplesImpl(trackEvents, project.config, &project, true, frames);
        for (const auto& clip : track.clips)
            renderAudioClip(trackAudio[trackIndex], clip, project, track.mixer);
        if (trackEvents.empty() && track.clips.empty()) continue;
        processEffectChain(trackAudio[trackIndex], track.inserts, project.config.sampleRate);
    }

    for (size_t trackIndex = 0; trackIndex < project.tracks.size(); ++trackIndex) {
        const auto& track = project.tracks[trackIndex];
        for (const auto& sidechain : track.sidechains) {
            auto source = std::find_if(project.tracks.begin(), project.tracks.end(),
                [&](const Track& candidate) { return candidate.id == sidechain.sourceTrackId; });
            if (source != project.tracks.end())
                processSidechain(trackAudio[trackIndex],
                    trackAudio[static_cast<size_t>(std::distance(project.tracks.begin(), source))],
                    sidechain, project.config.sampleRate);
        }
        addScaled(master, trackAudio[trackIndex], 1.0);
        for (const auto& send : track.sends) {
            auto bus = std::find_if(project.buses.begin(), project.buses.end(),
                [&](const Bus& candidate) { return candidate.id == send.busId; });
            if (bus != project.buses.end())
                addScaled(busAudio[static_cast<size_t>(std::distance(project.buses.begin(), bus))],
                          trackAudio[trackIndex], send.amount);
        }
    }

    for (size_t index = 0; index < project.buses.size(); ++index) {
        processEffectChain(busAudio[index], project.buses[index].inserts, project.config.sampleRate);
        addScaled(master, busAudio[index], project.buses[index].mixer.gain);
    }
    if (project.config.delayMix > 0.0) {
        Effect delay{EffectType::Delay, {{"time", project.config.delayBeats * 60000.0 / project.config.tempo},
                                         {"feedback", 0.38}}, project.config.delayMix};
        processEffect(master, delay, project.config.sampleRate);
    }
    if (project.config.reverb > 0.0) {
        Effect reverb{EffectType::Reverb, {}, project.config.reverb};
        processEffect(master, reverb, project.config.sampleRate);
    }
    processEffectChain(master, project.master.inserts, project.config.sampleRate);

    double peak = 0.0;
    for (float sample : master.samples) peak = std::max(peak, std::fabs(static_cast<double>(sample)));
    const double target = std::pow(10.0, project.config.masterPeakDb / 20.0);
    const double gain = peak > 1e-12 ? target / peak : 1.0;
    for (float& sample : master.samples)
        sample = static_cast<float>(std::clamp(sample * gain, -1.0, 1.0));
    return master;
}

} // namespace lyra
