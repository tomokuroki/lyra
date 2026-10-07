#pragma once

#include <string>
#include <vector>

namespace lyra {

struct SourceAudio {
    std::vector<float> samples;
    int channels = 0;
    int sampleRate = 0;
};

SourceAudio loadAudioFile(const std::string& path);

} // namespace lyra
