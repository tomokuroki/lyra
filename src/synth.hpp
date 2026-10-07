#pragma once

#include "common.hpp"
#include "project.hpp"
#include <vector>

namespace lyra {

AudioBuffer generateSamples(const std::vector<NoteEvent>& events, const Config& cfg);
AudioBuffer generateSamples(const Project& project);

} // namespace lyra
