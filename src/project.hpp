#pragma once

#include "common.hpp"

#include <string>
#include <map>
#include <vector>

namespace lyra {

enum class AutomationCurve { Step, Linear };

enum class EffectType {
    LowPass, HighPass, ParametricEq, Distortion, Saturation, Bitcrusher,
    Chorus, Flanger, Phaser, Delay, Reverb,
    Compressor, Limiter, Gate, Expander, DeEsser,
    StereoWidth, AutoPan
};

struct Effect {
    EffectType type = EffectType::LowPass;
    std::map<std::string, double> parameters;
    double wet = 1.0;
};

struct Send {
    std::string busId;
    double amount = 0.0;
};

struct Sidechain {
    std::string sourceTrackId;
    double amount = 1.0;
    double thresholdDb = -18.0;
    double ratio = 4.0;
    double attackMs = 10.0;
    double releaseMs = 120.0;
};

struct AutomationPoint {
    double beat = 0.0;
    double value = 0.0;
    AutomationCurve curve = AutomationCurve::Linear;
    int sourceLine = 0;
};

struct AutomationLane {
    std::string trackId;
    std::string parameter;
    std::vector<AutomationPoint> points;
};

struct Marker {
    std::string name;
    double beat = 0.0;
    int sourceLine = 0;
};

struct TempoPoint {
    double beat = 0.0;
    double bpm = 120.0;
    int sourceLine = 0;
};

struct Section {
    std::string name;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    int sourceLine = 0;
};

struct PatternPlacement {
    std::string pattern;
    std::string trackId;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    int sourceLine = 0;
};

struct TimelineSettings {
    double gridBeats = 0.0;
    double swingPercent = 50.0;
};

struct MixerChannel {
    double gain = 1.0;
    double pan = 0.0;
    bool mute = false;
    bool solo = false;
};

struct AudioClip {
    std::string path;
    std::string trackId;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    double trimStartSeconds = 0.0;
    double trimEndSeconds = -1.0;
    double fadeInBeats = 0.0;
    double fadeOutBeats = 0.0;
    double gain = 1.0;
    double pitchSemitones = 0.0;
    double stretch = 1.0;
    bool reverse = false;
    bool loop = false;
    double crossfadeMs = 0.0;
    int sourceLine = 0;
};

struct Track {
    std::string id;
    std::string name;
    MixerChannel mixer;
    std::vector<Effect> inserts;
    std::vector<Send> sends;
    std::vector<Sidechain> sidechains;
    std::vector<AudioClip> clips;
    std::vector<NoteEvent> events;
};

struct Bus {
    std::string id;
    MixerChannel mixer;
    std::vector<Effect> inserts;
};

struct MasterChannel {
    std::vector<Effect> inserts;
};

// The durable intermediate representation of a Lyra song.  Language syntax
// lowers into Project; renderers consume the flattened event view.  Keeping
// the musical model here prevents parser syntax, DSP and exporters from
// becoming coupled as arrangement features grow.
struct Project {
    Config config;
    std::vector<Track> tracks;
    std::vector<AutomationLane> automation;
    std::vector<Marker> markers;
    std::vector<TempoPoint> tempoMap;
    std::vector<Section> sections;
    std::vector<PatternPlacement> arrangement;
    std::vector<Bus> buses;
    MasterChannel master;
    TimelineSettings timeline;

    void validate() const;
    std::vector<NoteEvent> renderEvents() const;
    double durationBeats() const;
    double beatToSeconds(double beat) const;
    double durationSeconds(double startBeat, double durationBeats) const;
};

EffectType parseEffectType(const std::string& name);
std::string effectTypeToString(EffectType type);

} // namespace lyra
