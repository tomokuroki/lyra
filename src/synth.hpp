#pragma once

#include "common.hpp"
#include <vector>

namespace lyra {

AudioBuffer generateSamples(const std::vector<NoteEvent>& events, const Config& cfg);

} // namespace lyra
