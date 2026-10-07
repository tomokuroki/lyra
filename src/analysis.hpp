#pragma once

#include "common.hpp"

#include <string>
#include <vector>

namespace lyra {

struct AudioAnalysis {
    double peakDbfs = -120.0;
    double rmsDbfs = -120.0;
    double integratedLufs = -120.0;
    double stereoCorrelation = 1.0;
    size_t clippedSamples = 0;
    std::vector<double> spectrumDb;
    std::vector<double> waveformPeaks;
};

AudioAnalysis analyzeAudio(const AudioBuffer& audio, int sampleRate);
void writeAnalysisJson(const std::string& filename, const AudioAnalysis& analysis,
                       int sampleRate, int channels);

} // namespace lyra
