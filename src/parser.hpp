#pragma once

#include "common.hpp"
#include "project.hpp"
#include <string>
#include <vector>

namespace lyra {

class Parser {
public:
    std::vector<NoteEvent> events;
    Config config;
    Project project;

    void parse(const std::string& source);
};

} // namespace lyra
