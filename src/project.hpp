#pragma once

#include "common.hpp"

#include <string>
#include <vector>

namespace lyra {

enum class AutomationCurve { Step, Linear };

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

struct MixerChannel {
    double gain = 1.0;
    double pan = 0.0;
    bool mute = false;
    bool solo = false;
};

struct Track {
    std::string id;
    std::string name;
    MixerChannel mixer;
    std::vector<NoteEvent> events;
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

    void validate() const;
    std::vector<NoteEvent> renderEvents() const;
    double durationBeats() const;
};

} // namespace lyra
