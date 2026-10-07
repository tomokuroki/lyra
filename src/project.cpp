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

EffectType parseEffectType(const std::string& rawName) {
    std::string name = toLower(rawName);
    std::replace(name.begin(), name.end(), '-', '_');
    if (name == "lowpass" || name == "lp") return EffectType::LowPass;
    if (name == "highpass" || name == "hp") return EffectType::HighPass;
    if (name == "distortion" || name == "drive") return EffectType::Distortion;
    if (name == "saturation" || name == "saturator") return EffectType::Saturation;
    if (name == "bitcrusher" || name == "crusher") return EffectType::Bitcrusher;
    if (name == "chorus") return EffectType::Chorus;
    if (name == "delay" || name == "echo") return EffectType::Delay;
    if (name == "reverb") return EffectType::Reverb;
    if (name == "compressor" || name == "comp") return EffectType::Compressor;
    if (name == "limiter") return EffectType::Limiter;
    if (name == "gate") return EffectType::Gate;
    if (name == "stereo_width" || name == "widener") return EffectType::StereoWidth;
    if (name == "auto_pan" || name == "autopan") return EffectType::AutoPan;
    throw std::runtime_error("Unknown effect: " + rawName);
}

std::string effectTypeToString(EffectType type) {
    switch (type) {
        case EffectType::LowPass: return "lowpass";
        case EffectType::HighPass: return "highpass";
        case EffectType::Distortion: return "distortion";
        case EffectType::Saturation: return "saturation";
        case EffectType::Bitcrusher: return "bitcrusher";
        case EffectType::Chorus: return "chorus";
        case EffectType::Delay: return "delay";
        case EffectType::Reverb: return "reverb";
        case EffectType::Compressor: return "compressor";
        case EffectType::Limiter: return "limiter";
        case EffectType::Gate: return "gate";
        case EffectType::StereoWidth: return "stereo_width";
        case EffectType::AutoPan: return "auto_pan";
    }
    return "lowpass";
}

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
    std::set<std::string> busIds;
    for (const auto& bus : buses) {
        if (bus.id.empty() || !busIds.insert(bus.id).second)
            throw std::runtime_error("Duplicate or empty bus id: " + bus.id);
    }
    for (const auto& track : tracks)
        for (const auto& send : track.sends)
            if (!busIds.count(send.busId) || send.amount < 0.0 || send.amount > 1.0)
                throw std::runtime_error("Invalid send from " + track.name + " to " + send.busId);
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
    double previousTempoBeat = -1.0;
    for (const auto& point : tempoMap) {
        if (!std::isfinite(point.beat) || point.beat < 0.0 ||
            !std::isfinite(point.bpm) || point.bpm <= 0.0 ||
            point.beat < previousTempoBeat)
            throw std::runtime_error("Tempo points must be ordered and have BPM > 0");
        previousTempoBeat = point.beat;
    }
    if (timeline.gridBeats < 0.0 || !std::isfinite(timeline.gridBeats) ||
        timeline.swingPercent < 50.0 || timeline.swingPercent > 75.0)
        throw std::runtime_error("Invalid grid or swing settings");
    for (const auto& section : sections) {
        if (section.name.empty() || section.startBeat < 0.0 || section.lengthBeats <= 0.0)
            throw std::runtime_error("Invalid section: " + section.name);
    }
    for (const auto& placement : arrangement) {
        if (!ids.count(placement.trackId) || placement.pattern.empty() ||
            placement.startBeat < 0.0 || placement.lengthBeats < 0.0)
            throw std::runtime_error("Invalid pattern placement: " + placement.pattern);
    }
}

std::vector<NoteEvent> Project::renderEvents() const {
    const bool hasSolo = std::any_of(tracks.begin(), tracks.end(),
        [](const Track& track) { return track.mixer.solo; });
    std::vector<NoteEvent> result;
    for (const auto& track : tracks) {
        if (track.mixer.mute || (hasSolo && !track.mixer.solo)) continue;
        for (auto event : track.events) {
            if (timeline.gridBeats > 0.0) {
                const double cell = std::round(event.startBeat / timeline.gridBeats);
                event.startBeat = cell * timeline.gridBeats;
                const auto cellIndex = static_cast<long long>(std::llround(cell));
                if ((cellIndex & 1LL) != 0)
                    event.startBeat += timeline.gridBeats * (timeline.swingPercent / 100.0 - 0.5);
            }
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

double Project::beatToSeconds(double beat) const {
    if (beat <= 0.0) return 0.0;
    double cursorBeat = 0.0;
    double seconds = 0.0;
    double bpm = config.tempo;
    for (const auto& point : tempoMap) {
        if (point.beat > beat) break;
        if (point.beat > cursorBeat)
            seconds += (point.beat - cursorBeat) * 60.0 / bpm;
        cursorBeat = point.beat;
        bpm = point.bpm;
    }
    return seconds + (beat - cursorBeat) * 60.0 / bpm;
}

double Project::durationSeconds(double startBeat, double durationBeats) const {
    return beatToSeconds(startBeat + durationBeats) - beatToSeconds(startBeat);
}

} // namespace lyra
