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
    Noise,
    PinkNoise,
    BrownNoise,
    BlueNoise
};

enum class FilterType { LowPass, HighPass, BandPass, Notch };
enum class LfoWave { Sine, Triangle, Random };
enum class ModTarget { Pitch, Cutoff, Pan, Amp };

struct LfoRoute {
    LfoWave wave = LfoWave::Sine;
    ModTarget target = ModTarget::Pitch;
    double rateHz = 1.0;
    double amount = 0.0;
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
    OGG,
    AAC
};

enum class ScaleType {
    Major, NaturalMinor, HarmonicMinor, MelodicMinor,
    Dorian, Phrygian, Lydian, Mixolydian, Locrian,
    MajorPentatonic, MinorPentatonic, Blues, Chromatic
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
    double decay = -1.0;
    double sustain = -1.0;
    double release = -1.0;
    double cutoff = 0.0;
    FilterType filterType = FilterType::LowPass;
    double resonance = 0.0;
    double pitchEnvelopeStart = 0.0;
    double pitchEnvelopeEnd = 0.0;
    double filterEnvelopeStart = 0.0;
    double filterEnvelopeEnd = 0.0;
    std::vector<LfoRoute> lfoRoutes;
    double drive = 0.0;
    struct Oscillator {
        WaveType wave = WaveType::Sine;
        double level = 1.0;
        double semitones = 0.0;
        double detuneCents = 0.0;
    };
    std::vector<Oscillator> oscillators;
    int unisonVoices = 1;
    double unisonDetuneCents = 0.0;
    double fmRatio = 0.0;
    double fmAmount = 0.0;
    double amRate = 0.0;
    double amDepth = 0.0;
    double probability = 1.0;
    bool triggered = true;
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
    ScaleType scale = ScaleType::Major;
    bool scaleLock = false;
    uint32_t randomSeed = 1;
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
    if (format == "aac" || format == "m4a") return ExportFormat::AAC;
    throw std::runtime_error("Unknown export format: " + value +
        ". Expected wav, midi, aiff, json, flac, mp3, ogg, or aac");
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
        case ExportFormat::AAC: return "AAC";
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
        case ExportFormat::AAC: return ".m4a";
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
    if (w == "pink" || w == "pink_noise") return WaveType::PinkNoise;
    if (w == "brown" || w == "brown_noise") return WaveType::BrownNoise;
    if (w == "blue" || w == "blue_noise") return WaveType::BlueNoise;
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
        case WaveType::PinkNoise:return "pink_noise";
        case WaveType::BrownNoise:return "brown_noise";
        case WaveType::BlueNoise:return "blue_noise";
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

inline ScaleType parseScaleType(std::string name) {
    name = toLower(name);
    std::replace(name.begin(), name.end(), '-', '_');
    if (name == "major" || name == "ionian") return ScaleType::Major;
    if (name == "minor" || name == "natural_minor" || name == "aeolian") return ScaleType::NaturalMinor;
    if (name == "harmonic_minor") return ScaleType::HarmonicMinor;
    if (name == "melodic_minor") return ScaleType::MelodicMinor;
    if (name == "dorian") return ScaleType::Dorian;
    if (name == "phrygian") return ScaleType::Phrygian;
    if (name == "lydian") return ScaleType::Lydian;
    if (name == "mixolydian") return ScaleType::Mixolydian;
    if (name == "locrian") return ScaleType::Locrian;
    if (name == "major_pentatonic") return ScaleType::MajorPentatonic;
    if (name == "minor_pentatonic") return ScaleType::MinorPentatonic;
    if (name == "blues") return ScaleType::Blues;
    if (name == "chromatic") return ScaleType::Chromatic;
    throw std::runtime_error("Unknown scale: " + name);
}

inline std::string scaleTypeToString(ScaleType scale) {
    switch (scale) {
        case ScaleType::Major: return "major";
        case ScaleType::NaturalMinor: return "minor";
        case ScaleType::HarmonicMinor: return "harmonic_minor";
        case ScaleType::MelodicMinor: return "melodic_minor";
        case ScaleType::Dorian: return "dorian";
        case ScaleType::Phrygian: return "phrygian";
        case ScaleType::Lydian: return "lydian";
        case ScaleType::Mixolydian: return "mixolydian";
        case ScaleType::Locrian: return "locrian";
        case ScaleType::MajorPentatonic: return "major_pentatonic";
        case ScaleType::MinorPentatonic: return "minor_pentatonic";
        case ScaleType::Blues: return "blues";
        case ScaleType::Chromatic: return "chromatic";
    }
    return "major";
}

inline const std::vector<int>& scaleIntervals(ScaleType scale) {
    static const std::vector<int> major{0,2,4,5,7,9,11};
    static const std::vector<int> naturalMinor{0,2,3,5,7,8,10};
    static const std::vector<int> harmonicMinor{0,2,3,5,7,8,11};
    static const std::vector<int> melodicMinor{0,2,3,5,7,9,11};
    static const std::vector<int> dorian{0,2,3,5,7,9,10};
    static const std::vector<int> phrygian{0,1,3,5,7,8,10};
    static const std::vector<int> lydian{0,2,4,6,7,9,11};
    static const std::vector<int> mixolydian{0,2,4,5,7,9,10};
    static const std::vector<int> locrian{0,1,3,5,6,8,10};
    static const std::vector<int> majorPentatonic{0,2,4,7,9};
    static const std::vector<int> minorPentatonic{0,3,5,7,10};
    static const std::vector<int> blues{0,3,5,6,7,10};
    static const std::vector<int> chromatic{0,1,2,3,4,5,6,7,8,9,10,11};
    switch (scale) {
        case ScaleType::Major: return major;
        case ScaleType::NaturalMinor: return naturalMinor;
        case ScaleType::HarmonicMinor: return harmonicMinor;
        case ScaleType::MelodicMinor: return melodicMinor;
        case ScaleType::Dorian: return dorian;
        case ScaleType::Phrygian: return phrygian;
        case ScaleType::Lydian: return lydian;
        case ScaleType::Mixolydian: return mixolydian;
        case ScaleType::Locrian: return locrian;
        case ScaleType::MajorPentatonic: return majorPentatonic;
        case ScaleType::MinorPentatonic: return minorPentatonic;
        case ScaleType::Blues: return blues;
        case ScaleType::Chromatic: return chromatic;
    }
    return major;
}

inline int noteToMidi(const std::string& name) {
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

    return (octave + 1) * 12 + it->second;
}

inline double midiToFreq(double midi) {
    return 440.0 * std::pow(2.0, (midi - 69.0) / 12.0);
}

inline double noteToFreq(const std::string& name) {
    return midiToFreq(noteToMidi(name));
}

inline int lockMidiToScale(int midi, int root, ScaleType scale) {
    const auto& intervals = scaleIntervals(scale);
    int best = midi;
    int bestDistance = 13;
    for (int candidate = midi - 6; candidate <= midi + 6; ++candidate) {
        const int relative = ((candidate - root) % 12 + 12) % 12;
        if (std::find(intervals.begin(), intervals.end(), relative) != intervals.end()) {
            const int distance = std::abs(candidate - midi);
            if (distance < bestDistance) { best = candidate; bestDistance = distance; }
        }
    }
    return best;
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
    else if (quality == "6" || quality == "maj6") intervals = {0,4,7,9};
    else if (quality == "m6" || quality == "min6") intervals = {0,3,7,9};
    else if (quality == "9") intervals = {0,4,7,10,14};
    else if (quality == "maj9") intervals = {0,4,7,11,14};
    else if (quality == "m9" || quality == "min9") intervals = {0,3,7,10,14};
    else if (quality == "add9") intervals = {0,4,7,14};
    else if (quality == "11") intervals = {0,4,7,10,14,17};
    else if (quality == "13") intervals = {0,4,7,10,14,17,21};
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
