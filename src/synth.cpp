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

static void renderDrum(const NoteEvent& ev, size_t startS, size_t nS,
                       std::vector<double>& mix, int sampleRate) {
    for (size_t i = 0; i < nS && startS + i < mix.size(); ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double env = 1.0;
        double sample = 0.0;

        switch (ev.drum) {
            case DrumType::Kick: {
                double freq = 150.0 * std::exp(-t * 25.0);
                env = std::exp(-t * 12.0);
                sample = std::sin(2.0 * PI * freq * t) * 0.9
                       + sampleWave(WaveType::Noise, t * 3.0, t) * 0.3;
                break;
            }
            case DrumType::Snare: {
                env = std::exp(-t * 18.0);
                sample = sampleWave(WaveType::Noise, t * 8.0, t) * 0.85
                       + std::sin(2.0 * PI * 200.0 * t) * 0.25 * std::exp(-t * 30.0);
                break;
            }
            case DrumType::Hihat: {
                env = std::exp(-t * 40.0);
                sample = sampleWave(WaveType::Noise, t * 20.0, t) * 0.55;
                break;
            }
            case DrumType::Tom: {
                double freq = 120.0 * std::exp(-t * 8.0);
                env = std::exp(-t * 9.0);
                sample = std::sin(2.0 * PI * freq * t);
                break;
            }
            default: break;
        }
        mix[startS + i] += sample * env * ev.volume;
    }
}

std::vector<int16_t> generateSamples(const std::vector<NoteEvent>& events, const Config& cfg) {
    if (events.empty()) return {};

    double maxBeat = 0.0;
    for (const auto& e : events)
        maxBeat = std::max(maxBeat, e.startBeat + e.durationBeats);

    double totalSec = maxBeat * (60.0 / cfg.tempo);
    size_t totalSamples = static_cast<size_t>(totalSec * cfg.sampleRate + 0.5);
    std::vector<double> mix(totalSamples, 0.0);

    for (const auto& ev : events) {
        size_t startS = static_cast<size_t>(ev.startBeat * (60.0 / cfg.tempo) * cfg.sampleRate);
        size_t nS = static_cast<size_t>(ev.durationBeats * (60.0 / cfg.tempo) * cfg.sampleRate + 0.5);
        if (nS == 0) continue;

        if (ev.drum != DrumType::None) {
            renderDrum(ev, startS, nS, mix, cfg.sampleRate);
            continue;
        }

        size_t voices = std::min(ev.freqs.size(), size_t(8));
        if (voices == 0) continue;

        double durSec = ev.durationBeats * (60.0 / cfg.tempo);

        for (size_t i = 0; i < nS && startS + i < totalSamples; ++i) {
            double t = static_cast<double>(i) / cfg.sampleRate;
            double env = 1.0;
            const double atk = 0.008, rel = 0.05;
            if (t < atk) env = t / atk;
            else if (t > durSec - rel) env = std::max(0.0, (durSec - t) / rel);

            double sample = 0.0;
            for (size_t v = 0; v < voices; ++v) {
                double f = ev.freqs[v];
                if (f <= 0.0) continue;
                double phase = std::fmod(f * t, 1.0);
                sample += sampleWave(ev.wave, phase, t) * (1.0 / voices);
            }
            mix[startS + i] += sample * env * ev.volume;
        }
    }

    double peak = 0.0;
    for (double s : mix) peak = std::max(peak, std::fabs(s));
    double norm = (peak > 0.95) ? 0.95 / peak : 1.0;

    std::vector<int16_t> out(totalSamples);
    for (size_t i = 0; i < totalSamples; ++i) {
        double s = std::max(-1.0, std::min(1.0, mix[i] * norm));
        out[i] = static_cast<int16_t>(s * 32767.0);
    }
    return out;
}

} // namespace lyra
