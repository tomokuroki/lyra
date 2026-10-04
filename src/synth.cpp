#include "synth.hpp"
#include <cmath>

namespace lyra {

static double sampleWave(WaveType wave, double phase, double t) {
    switch (wave) {
        case WaveType::Sine:     return std::sin(2.0 * PI * phase);
        case WaveType::Square:   return phase < 0.5 ? 1.0 : -1.0;
        case WaveType::Triangle: return 4.0 * std::fabs(phase - 0.5) - 1.0;
        case WaveType::Saw:      return 2.0 * phase - 1.0;
        case WaveType::Pulse:    return phase < 0.25 ? 1.0 : -1.0;
        case WaveType::Noise: {
            uint32_t x = static_cast<uint32_t>(t * 44100.0 * 7.0 + phase * 1e6);
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
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

AudioBuffer generateSamples(const std::vector<NoteEvent>& events, const Config& cfg) {
    if (events.empty()) return AudioBuffer{{}, cfg.channels};

    double maxBeat = 0.0;
    for (const auto& e : events)
        maxBeat = std::max(maxBeat, e.startBeat + e.durationBeats);

    double beatSec = 60.0 / cfg.tempo;
    double effectTail = (cfg.reverb > 0.0 ? 1.4 : 0.0)
                      + (cfg.delayMix > 0.0 ? cfg.delayBeats * beatSec * 2.0 : 0.0);
    double totalSec = maxBeat * beatSec + effectTail;
    size_t totalSamples = static_cast<size_t>(totalSec * cfg.sampleRate + 0.5);
    std::vector<double> mixL(totalSamples, 0.0);
    std::vector<double> mixR(totalSamples, 0.0);

    for (const auto& ev : events) {
        size_t startS = static_cast<size_t>(ev.startBeat * (60.0 / cfg.tempo) * cfg.sampleRate);
        size_t nS = static_cast<size_t>(ev.durationBeats * (60.0 / cfg.tempo) * cfg.sampleRate + 0.5);
        if (nS == 0) continue;

        if (ev.drum != DrumType::None) {
            renderDrum(ev, startS, nS, mixL, mixR, cfg.sampleRate);
            continue;
        }

        size_t voices = std::min(ev.freqs.size(), size_t(8));
        if (voices == 0) continue;

        double durSec = ev.durationBeats * (60.0 / cfg.tempo);
        double filterState = 0.0;
        double filterAlpha = ev.cutoff > 0.0
            ? std::min(1.0, 1.0 - std::exp(-2.0 * PI * ev.cutoff / cfg.sampleRate)) : 1.0;

        for (size_t i = 0; i < nS && startS + i < totalSamples; ++i) {
            double t = static_cast<double>(i) / cfg.sampleRate;
            double env = instrumentEnvelope(ev, t, durSec);

            double sample = 0.0;
            for (size_t v = 0; v < voices; ++v) {
                double f = ev.freqs[v];
                if (f <= 0.0) continue;
                double phase = std::fmod(f * t, 1.0);
                sample += instrumentSample(ev, phase, t, f, cfg.soundMode) * (1.0 / voices);
            }
            if (ev.drive > 0.0) {
                double amount = 1.0 + ev.drive * 5.0;
                sample = std::tanh(sample * amount) / std::tanh(amount);
            }
            filterState += filterAlpha * (sample - filterState);
            sample = filterState;
            double panAngle = (ev.pan + 1.0) * PI * 0.25;
            double value = sample * env * ev.volume;
            mixL[startS + i] += value * std::cos(panAngle);
            mixR[startS + i] += value * std::sin(panAngle);
        }
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

} // namespace lyra
