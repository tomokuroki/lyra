#pragma once

#include "common.hpp"
#include <string>
#include <vector>

namespace lyra {

void writeWav(const std::string& filename, const AudioBuffer& audio, const Config& cfg);
void writeMidi(const std::string& filename, const std::vector<NoteEvent>& events, const Config& cfg);
void writeAiff(const std::string& filename, const AudioBuffer& audio, const Config& cfg);
void writeJson(const std::string& filename, const std::vector<NoteEvent>& events, const Config& cfg);
void writeCompressedAudio(const std::string& filename, const AudioBuffer& audio,
                          const Config& cfg, ExportFormat format);

} // namespace lyra
