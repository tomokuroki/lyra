#pragma once

#include "common.hpp"
#include "project.hpp"

#include <vector>

namespace lyra {

void processEffect(AudioBuffer& audio, const Effect& effect, int sampleRate);
void processEffectChain(AudioBuffer& audio, const std::vector<Effect>& effects, int sampleRate);
void processSidechain(AudioBuffer& target, const AudioBuffer& key,
                      const Sidechain& settings, int sampleRate);

} // namespace lyra
