#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace lyra {

// Lyra 2 language front-end. It expands high-level language constructs into
// the compact command stream consumed by the audio parser.
class Frontend {
public:
    std::string processFile(const std::string& filename);

private:
    struct Macro {
        std::vector<std::string> params;
        std::vector<std::string> body;
    };

    struct Preset {
        std::vector<std::string> body;
    };

    std::map<std::string, std::string> variables_;
    std::set<std::string> constants_;
    std::map<std::string, Macro> patterns_;
    std::map<std::string, Macro> functions_;
    std::map<std::string, Preset> instruments_;
    std::map<std::string, Preset> effects_;
    std::set<std::filesystem::path> importStack_;
    std::set<std::filesystem::path> imported_;

    std::vector<std::string> processPath(const std::filesystem::path& path);
    std::vector<std::string> expand(const std::vector<std::string>& lines,
                                    const std::filesystem::path& baseDir,
                                    const std::map<std::string, std::string>& locals = {},
                                    int transpose = 0);
};

} // namespace lyra
