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

enum class InstrumentType {
    Wave,
    Piano,
    ElectricPiano,
    Organ,
    MusicBox,
    Glockenspiel,
    Strings,
    Brass,
    Flute,
    Guitar,
    ElectricGuitar,
    SynthBass
};

enum class DrumType {
    None,
    Kick,
    Snare,
    Hihat,
    OpenHihat,
    TomLow,
    TomMid,
    TomHigh,
    Clap,
    Rimshot,
    Crash,
    Ride,
    Cowbell,
    Shaker,
    Tambourine,
    Timpani,
    Impact
};

enum class DrumKit {
    Standard,
    Rock,
    Electronic,
    Retro,
    Orchestral
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
    InstrumentType instrument = InstrumentType::Wave;
    DrumType drum = DrumType::None;
    DrumKit drumKit = DrumKit::Standard;
};

struct Config {
    double tempo = 120.0;
    double volume = 0.7;
    WaveType wave = WaveType::Square;
    InstrumentType instrument = InstrumentType::Wave;
    DrumKit drumKit = DrumKit::Standard;
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

inline InstrumentType parseInstrument(const std::string& s) {
    std::string name = toLower(s);
    std::replace(name.begin(), name.end(), '-', '_');
    if (name == "piano" || name == "acoustic_piano" || name == "grand_piano") return InstrumentType::Piano;
    if (name == "epiano" || name == "electric_piano" || name == "electricpiano") return InstrumentType::ElectricPiano;
    if (name == "organ" || name == "church_organ") return InstrumentType::Organ;
    if (name == "musicbox" || name == "music_box") return InstrumentType::MusicBox;
    if (name == "glockenspiel" || name == "glock") return InstrumentType::Glockenspiel;
    if (name == "strings" || name == "string") return InstrumentType::Strings;
    if (name == "brass" || name == "horn") return InstrumentType::Brass;
    if (name == "flute") return InstrumentType::Flute;
    if (name == "guitar" || name == "acoustic_guitar") return InstrumentType::Guitar;
    if (name == "electric_guitar" || name == "eguitar" || name == "distorted_guitar") return InstrumentType::ElectricGuitar;
    if (name == "synth_bass" || name == "synthbass" || name == "bass") return InstrumentType::SynthBass;
    // Wave names remain valid after `instrument` for backwards compatibility.
    parseWave(name);
    return InstrumentType::Wave;
}

inline DrumKit parseDrumKit(const std::string& s) {
    std::string name = toLower(s);
    if (name == "standard" || name == "acoustic") return DrumKit::Standard;
    if (name == "rock") return DrumKit::Rock;
    if (name == "electronic" || name == "electro") return DrumKit::Electronic;
    if (name == "retro" || name == "8bit" || name == "chiptune") return DrumKit::Retro;
    if (name == "orchestral" || name == "orchestra") return DrumKit::Orchestral;
    throw std::runtime_error("Unknown drum kit: " + s);
}

inline DrumType parseDrum(const std::string& s) {
    std::string name = toLower(s);
    std::replace(name.begin(), name.end(), '-', '_');
    if (name == "kick" || name == "bassdrum") return DrumType::Kick;
    if (name == "snare") return DrumType::Snare;
    if (name == "hihat" || name == "closed_hihat" || name == "hat") return DrumType::Hihat;
    if (name == "open_hihat" || name == "openhat") return DrumType::OpenHihat;
    if (name == "tom" || name == "mid_tom" || name == "tom_mid") return DrumType::TomMid;
    if (name == "low_tom" || name == "tom_low") return DrumType::TomLow;
    if (name == "high_tom" || name == "tom_high") return DrumType::TomHigh;
    if (name == "clap") return DrumType::Clap;
    if (name == "rimshot" || name == "rim") return DrumType::Rimshot;
    if (name == "crash") return DrumType::Crash;
    if (name == "ride") return DrumType::Ride;
    if (name == "cowbell") return DrumType::Cowbell;
    if (name == "shaker") return DrumType::Shaker;
    if (name == "tambourine" || name == "tamb") return DrumType::Tambourine;
    if (name == "timpani") return DrumType::Timpani;
    if (name == "impact") return DrumType::Impact;
    throw std::runtime_error("Unknown drum: " + s);
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
