#include "project.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace lyra {
namespace {

double valueAt(const AutomationLane& lane, double beat) {
    if (lane.points.empty()) return 0.0;
    if (beat <= lane.points.front().beat) return lane.points.front().value;
    for (size_t i = 1; i < lane.points.size(); ++i) {
        const auto& left = lane.points[i - 1];
        const auto& right = lane.points[i];
        if (beat <= right.beat) {
            if (left.curve == AutomationCurve::Step || right.beat == left.beat)
                return left.value;
            const double t = (beat - left.beat) / (right.beat - left.beat);
            return left.value + (right.value - left.value) * t;
        }
    }
    return lane.points.back().value;
}

} // namespace

void Project::validate() const {
    if (!(config.tempo > 0.0) || !std::isfinite(config.tempo))
        throw std::runtime_error("Project tempo must be finite and greater than zero");

    std::set<std::string> ids;
    for (const auto& track : tracks) {
        if (track.id.empty()) throw std::runtime_error("Track id cannot be empty");
        if (!ids.insert(track.id).second)
            throw std::runtime_error("Duplicate track id: " + track.id);
        if (!std::isfinite(track.mixer.gain) || track.mixer.gain < 0.0)
            throw std::runtime_error("Invalid mixer gain on track: " + track.name);
        for (const auto& event : track.events) {
            if (!std::isfinite(event.startBeat) || event.startBeat < 0.0 ||
                !std::isfinite(event.durationBeats) || event.durationBeats <= 0.0)
                throw std::runtime_error("Invalid event timing on track: " + track.name);
        }
    }
    for (const auto& lane : automation) {
        if (!ids.count(lane.trackId))
            throw std::runtime_error("Automation targets unknown track: " + lane.trackId);
        if (lane.parameter != "volume" && lane.parameter != "pan" &&
            lane.parameter != "cutoff" && lane.parameter != "drive")
            throw std::runtime_error("Unknown automation parameter: " + lane.parameter);
        double previous = -1.0;
        for (const auto& point : lane.points) {
            if (!std::isfinite(point.beat) || !std::isfinite(point.value) ||
                point.beat < 0.0 || point.beat < previous)
                throw std::runtime_error("Automation points must be ordered by beat");
            previous = point.beat;
        }
    }
}

std::vector<NoteEvent> Project::renderEvents() const {
    const bool hasSolo = std::any_of(tracks.begin(), tracks.end(),
        [](const Track& track) { return track.mixer.solo; });
    std::vector<NoteEvent> result;
    for (const auto& track : tracks) {
        if (track.mixer.mute || (hasSolo && !track.mixer.solo)) continue;
        for (auto event : track.events) {
            event.volume *= track.mixer.gain;
            event.pan = std::clamp(event.pan + track.mixer.pan, -1.0, 1.0);
            for (const auto& lane : automation) {
                if (lane.trackId != track.id || lane.points.empty()) continue;
                const double value = valueAt(lane, event.startBeat);
                if (lane.parameter == "volume") event.volume *= value / 100.0;
                else if (lane.parameter == "pan") event.pan = std::clamp(value / 100.0, -1.0, 1.0);
                else if (lane.parameter == "cutoff") event.cutoff = value;
                else if (lane.parameter == "drive") event.drive = value / 100.0;
            }
            result.push_back(event);
        }
    }
    std::stable_sort(result.begin(), result.end(), [](const NoteEvent& a, const NoteEvent& b) {
        return a.startBeat < b.startBeat;
    });
    return result;
}

double Project::durationBeats() const {
    double duration = 0.0;
    for (const auto& track : tracks)
        for (const auto& event : track.events)
            duration = std::max(duration, event.startBeat + event.durationBeats);
    return duration;
}

} // namespace lyra
