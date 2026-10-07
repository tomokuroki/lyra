#include "analysis.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace lyra {

AudioAnalysis analyzeAudio(const AudioBuffer& audio, int sampleRate) {
    AudioAnalysis report;
    if (audio.samples.empty() || audio.channels <= 0) return report;
    double peak = 0.0, sumSquares = 0.0;
    for (float sample : audio.samples) {
        peak = std::max(peak, std::fabs(static_cast<double>(sample)));
        sumSquares += static_cast<double>(sample) * sample;
        if (std::fabs(sample) >= 1.0f) ++report.clippedSamples;
    }
    const double meanSquare = sumSquares / audio.samples.size();
    report.peakDbfs = 20.0 * std::log10(std::max(peak, 1e-6));
    report.rmsDbfs = 10.0 * std::log10(std::max(meanSquare, 1e-12));
    report.integratedLufs = -0.691 + 10.0 * std::log10(std::max(meanSquare, 1e-12));

    if (audio.channels == 2) {
        double lr = 0.0, ll = 0.0, rr = 0.0;
        for (size_t i = 0; i + 1 < audio.samples.size(); i += 2) {
            const double left = audio.samples[i], right = audio.samples[i + 1];
            lr += left * right; ll += left * left; rr += right * right;
        }
        report.stereoCorrelation = lr / std::sqrt(std::max(1e-20, ll * rr));
    }

    constexpr size_t waveformPoints = 256;
    const size_t frames = audio.samples.size() / static_cast<size_t>(audio.channels);
    report.waveformPeaks.resize(waveformPoints, 0.0);
    for (size_t point = 0; point < waveformPoints; ++point) {
        const size_t begin = point * frames / waveformPoints;
        const size_t end = std::max(begin + 1, (point + 1) * frames / waveformPoints);
        for (size_t frame = begin; frame < end && frame < frames; ++frame)
            for (int channel = 0; channel < audio.channels; ++channel)
                report.waveformPeaks[point] = std::max(report.waveformPeaks[point],
                    std::fabs(static_cast<double>(audio.samples[frame * audio.channels + channel])));
    }

    constexpr size_t bins = 32;
    constexpr size_t window = 2048;
    report.spectrumDb.resize(bins, -120.0);
    if (frames > 0) {
        for (size_t bin = 0; bin < bins; ++bin) {
            const double frequency = 20.0 * std::pow((sampleRate * 0.5) / 20.0, bin / static_cast<double>(bins - 1));
            double real = 0.0, imaginary = 0.0;
            const size_t count = std::min(window, frames);
            for (size_t n = 0; n < count; ++n) {
                double mono = 0.0;
                for (int channel = 0; channel < audio.channels; ++channel)
                    mono += audio.samples[n * audio.channels + channel] / static_cast<double>(audio.channels);
                const double hann = count > 1 ? 0.5 - 0.5 * std::cos(2.0 * PI * n / (count - 1)) : 1.0;
                const double phase = 2.0 * PI * frequency * n / sampleRate;
                real += mono * hann * std::cos(phase); imaginary -= mono * hann * std::sin(phase);
            }
            const double magnitude = std::sqrt(real * real + imaginary * imaginary) / std::max<size_t>(1, count);
            report.spectrumDb[bin] = 20.0 * std::log10(std::max(magnitude, 1e-6));
        }
    }
    return report;
}

void writeAnalysisJson(const std::string& filename, const AudioAnalysis& report,
                       int sampleRate, int channels) {
    std::ofstream out(filename);
    if (!out) throw std::runtime_error("Cannot create analysis file: " + filename);
    out << std::setprecision(10) << "{\n"
        << "  \"sample_rate\": " << sampleRate << ",\n  \"channels\": " << channels
        << ",\n  \"peak_dbfs\": " << report.peakDbfs
        << ",\n  \"rms_dbfs\": " << report.rmsDbfs
        << ",\n  \"integrated_lufs\": " << report.integratedLufs
        << ",\n  \"stereo_correlation\": " << report.stereoCorrelation
        << ",\n  \"clipped_samples\": " << report.clippedSamples << ",\n  \"spectrum_db\": [";
    for (size_t i = 0; i < report.spectrumDb.size(); ++i) { if (i) out << ", "; out << report.spectrumDb[i]; }
    out << "],\n  \"waveform_peaks\": [";
    for (size_t i = 0; i < report.waveformPeaks.size(); ++i) { if (i) out << ", "; out << report.waveformPeaks[i]; }
    out << "]\n}\n";
}

} // namespace lyra
