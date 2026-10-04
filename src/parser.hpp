#pragma once

#include "common.hpp"
#include <string>
#include <vector>

namespace lyra {

class Parser {
public:
    std::vector<NoteEvent> events;
    Config config;

    void parse(const std::string& source);
};

} // namespace lyra
