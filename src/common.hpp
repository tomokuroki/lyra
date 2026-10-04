#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <cctype>

namespace lyra {

constexpr double PI = 3.14159265358979323846;
constexpr int DEFAULT_SAMPLE_RATE = 44100;

enum class WaveType {
    Sine,
    Square,
    Triangle,
    Saw,
    Pulse,
    Noise
};

enum class DrumType {
    None,
    Kick,
    Snare,
    Hihat,
    Tom
};

enum class ExportFormat {
    WAV,
    MIDI
};

struct NoteEvent {
    double startBeat = 0.0;
    std::vector<double> freqs;
    double durationBeats = 0.0;
    double volume = 0.7;
    WaveType wave = WaveType::Square;
    DrumType drum = DrumType::None;
};

struct Config {
    double tempo = 120.0;
    double volume = 0.7;
    WaveType wave = WaveType::Square;
    int sampleRate = DEFAULT_SAMPLE_RATE;
    int bits = 16;
    ExportFormat format = ExportFormat::WAV;
};

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

inline std::string toLower(std::string s) {
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline WaveType parseWave(const std::string& s) {
    std::string w = toLower(s);
    if (w == "sine" || w == "sin") return WaveType::Sine;
    if (w == "square" || w == "sq") return WaveType::Square;
    if (w == "triangle" || w == "tri") return WaveType::Triangle;
    if (w == "saw" || w == "sawtooth") return WaveType::Saw;
    if (w == "pulse") return WaveType::Pulse;
    if (w == "noise") return WaveType::Noise;
    throw std::runtime_error("Unknown wave type: " + s);
}

inline std::string waveToString(WaveType w) {
    switch (w) {
        case WaveType::Sine:     return "sine";
        case WaveType::Square:   return "square";
        case WaveType::Triangle: return "triangle";
        case WaveType::Saw:      return "saw";
        case WaveType::Pulse:    return "pulse";
        case WaveType::Noise:    return "noise";
    }
    return "square";
}

inline double noteToFreq(const std::string& name) {
    static const std::map<std::string, int> noteMap = {
        {"c",0},{"c#",1},{"db",1},{"d",2},{"d#",3},{"eb",3},
        {"e",4},{"f",5},{"f#",6},{"gb",6},{"g",7},{"g#",8},
        {"ab",8},{"a",9},{"a#",10},{"bb",10},{"b",11}
    };

    std::string n = toLower(name);
    if (n.empty()) return 0.0;

    size_t i = 0;
    std::string part;
    if (n[0] >= 'a' && n[0] <= 'g') {
        part += n[0];
        i = 1;
        if (i < n.size() && (n[i] == '#' || n[i] == 'b')) {
            part += n[i];
            ++i;
        }
    } else {
        throw std::runtime_error("Invalid note: " + name);
    }

    if (i >= n.size() || !std::isdigit(static_cast<unsigned char>(n[i])))
        throw std::runtime_error("Missing octave in note: " + name);

    int octave = std::stoi(n.substr(i));
    if (octave < 0 || octave > 8)
        throw std::runtime_error("Octave must be 0-8: " + name);

    auto it = noteMap.find(part);
    if (it == noteMap.end())
        throw std::runtime_error("Unknown note name: " + name);

    int midi = (octave + 1) * 12 + it->second;
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

} // namespace lyra
