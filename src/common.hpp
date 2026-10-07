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

enum class SoundMode {
    FourBit,
    EightBit,
    SixteenBit,
    ThirtyTwoBit,
    SixtyFourBit,
    Tracker,
    FM,
    ChiptuneModern,
    Modern
};

enum class ExportFormat {
    WAV,
    MIDI,
    AIFF,
    JSON,
    FLAC,
    MP3,
    OGG
};

struct NoteEvent {
    // Stable ownership information used by the project IR.  Exporters and the
    // synthesizer deliberately do not depend on it, so old integrations that
    // construct NoteEvent directly remain source compatible.
    std::string trackId;
    int sourceLine = 0;
    double startBeat = 0.0;
    std::vector<double> freqs;
    double durationBeats = 0.0;
    double volume = 0.7;
    WaveType wave = WaveType::Square;
    InstrumentType instrument = InstrumentType::Wave;
    DrumType drum = DrumType::None;
    DrumKit drumKit = DrumKit::Standard;
    double pan = 0.0;
    double attack = -1.0;
    double release = -1.0;
    double cutoff = 0.0;
    double drive = 0.0;
};

struct AudioBuffer {
    std::vector<float> samples;
    int channels = 2;
};

struct Config {
    double tempo = 120.0;
    double volume = 0.7;
    WaveType wave = WaveType::Square;
    InstrumentType instrument = InstrumentType::Wave;
    DrumKit drumKit = DrumKit::Standard;
    SoundMode soundMode = SoundMode::SixteenBit;
    double reverb = 0.0;
    double delayBeats = 0.0;
    double delayMix = 0.0;
    int channels = 2;
    double masterPeakDb = -1.0;
    double fadeInBeats = 0.0;
    double fadeOutBeats = 0.0;
    std::string title;
    std::string artist;
    std::string album;
    int timeNumerator = 4;
    int timeDenominator = 4;
    int keyRoot = 0;
    bool keyMinor = false;
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

inline ExportFormat parseExportFormat(const std::string& value) {
    std::string format = toLower(value);
    if (format == "wav" || format == "wave") return ExportFormat::WAV;
    if (format == "midi" || format == "mid") return ExportFormat::MIDI;
    if (format == "aiff" || format == "aif") return ExportFormat::AIFF;
    if (format == "json") return ExportFormat::JSON;
    if (format == "flac") return ExportFormat::FLAC;
    if (format == "mp3") return ExportFormat::MP3;
    if (format == "ogg" || format == "vorbis") return ExportFormat::OGG;
    throw std::runtime_error("Unknown export format: " + value +
        ". Expected wav, midi, aiff, json, flac, mp3, or ogg");
}

inline std::string exportFormatToString(ExportFormat format) {
    switch (format) {
        case ExportFormat::WAV: return "WAV";
        case ExportFormat::MIDI: return "MIDI";
        case ExportFormat::AIFF: return "AIFF";
        case ExportFormat::JSON: return "JSON";
        case ExportFormat::FLAC: return "FLAC";
        case ExportFormat::MP3: return "MP3";
        case ExportFormat::OGG: return "OGG";
    }
    return "WAV";
}

inline std::string exportFormatExtension(ExportFormat format) {
    switch (format) {
        case ExportFormat::WAV: return ".wav";
        case ExportFormat::MIDI: return ".mid";
        case ExportFormat::AIFF: return ".aiff";
        case ExportFormat::JSON: return ".json";
        case ExportFormat::FLAC: return ".flac";
        case ExportFormat::MP3: return ".mp3";
        case ExportFormat::OGG: return ".ogg";
    }
    return ".wav";
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

inline SoundMode parseSoundMode(const std::string& s) {
    std::string name = toLower(s);
    std::replace(name.begin(), name.end(), '-', '_');
    if (name == "4bit" || name == "4_bit" || name == "toy" || name == "primitive")
        return SoundMode::FourBit;
    if (name == "8bit" || name == "8_bit" || name == "retro" || name == "chiptune")
        return SoundMode::EightBit;
    if (name == "16bit" || name == "16_bit" || name == "snes")
        return SoundMode::SixteenBit;
    if (name == "32bit" || name == "32_bit" || name == "ps1" || name == "saturn")
        return SoundMode::ThirtyTwoBit;
    if (name == "64bit" || name == "64_bit" || name == "n64")
        return SoundMode::SixtyFourBit;
    if (name == "tracker" || name == "amiga" || name == "mod")
        return SoundMode::Tracker;
    if (name == "fm" || name == "fm_synthesis" || name == "genesis" || name == "mega_drive")
        return SoundMode::FM;
    if (name == "chiptune_modern" || name == "modern_chiptune" || name == "chipmodern")
        return SoundMode::ChiptuneModern;
    if (name == "modern" || name == "clean" || name == "hi_fi" || name == "hifi")
        return SoundMode::Modern;
    throw std::runtime_error("Unknown sound mode: " + s);
}

inline std::string soundModeToString(SoundMode mode) {
    switch (mode) {
        case SoundMode::FourBit: return "4bit";
        case SoundMode::EightBit: return "8bit";
        case SoundMode::SixteenBit: return "16bit";
        case SoundMode::ThirtyTwoBit: return "32bit";
        case SoundMode::SixtyFourBit: return "64bit";
        case SoundMode::Tracker: return "tracker";
        case SoundMode::FM: return "fm";
        case SoundMode::ChiptuneModern: return "chiptune_modern";
        case SoundMode::Modern: return "modern";
    }
    return "16bit";
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

inline int pitchClass(const std::string& name) {
    static const std::map<std::string, int> pitches = {
        {"c",0},{"c#",1},{"db",1},{"d",2},{"d#",3},{"eb",3},
        {"e",4},{"f",5},{"f#",6},{"gb",6},{"g",7},{"g#",8},
        {"ab",8},{"a",9},{"a#",10},{"bb",10},{"b",11}
    };
    auto it = pitches.find(toLower(name));
    if (it == pitches.end()) throw std::runtime_error("Unknown pitch class: " + name);
    return it->second;
}

inline std::vector<double> chordSymbolToFreqs(const std::string& symbol, int octave) {
    if (symbol.empty()) throw std::runtime_error("Empty chord symbol");
    size_t split = 1;
    if (symbol.size() > 1 && (symbol[1] == '#' || symbol[1] == 'b')) split = 2;
    int root = pitchClass(symbol.substr(0, split));
    std::string quality = toLower(symbol.substr(split));
    std::vector<int> intervals;
    if (quality.empty() || quality == "maj" || quality == "major") intervals = {0,4,7};
    else if (quality == "m" || quality == "min" || quality == "minor") intervals = {0,3,7};
    else if (quality == "7") intervals = {0,4,7,10};
    else if (quality == "maj7") intervals = {0,4,7,11};
    else if (quality == "m7" || quality == "min7") intervals = {0,3,7,10};
    else if (quality == "dim") intervals = {0,3,6};
    else if (quality == "dim7") intervals = {0,3,6,9};
    else if (quality == "aug" || quality == "+") intervals = {0,4,8};
    else if (quality == "sus2") intervals = {0,2,7};
    else if (quality == "sus4" || quality == "sus") intervals = {0,5,7};
    else if (quality == "5") intervals = {0,7};
    else throw std::runtime_error("Unknown chord quality: " + symbol);

    int rootMidi = (octave + 1) * 12 + root;
    std::vector<double> freqs;
    for (int interval : intervals) {
        int midi = rootMidi + interval;
        freqs.push_back(440.0 * std::pow(2.0, (midi - 69) / 12.0));
    }
    return freqs;
}

} // namespace lyra
