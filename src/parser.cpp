#include "parser.hpp"
#include <sstream>
#include <iostream>

namespace lyra {

void Parser::parse(const std::string& source) {
    events.clear();
    project = Project{};
    std::istringstream iss(source);
    std::string line;
    int lineNum = 0;

    double trackTime = 0.0;
    bool inTrack = false;
    WaveType trackWave = WaveType::Square;
    InstrumentType trackInstrument = InstrumentType::Wave;
    DrumKit trackDrumKit = DrumKit::Standard;
    double trackPan = 0.0;
    double trackAttack = -1.0;
    double trackDecay = -1.0;
    double trackSustain = -1.0;
    double trackRelease = -1.0;
    double trackCutoff = 0.0;
    double trackDrive = 0.0;
    double trackVol = 0.7;
    std::vector<NoteEvent::Oscillator> trackOscillators;
    int trackUnisonVoices = 1;
    double trackUnisonDetune = 0.0;
    double trackFmRatio = 0.0, trackFmAmount = 0.0;
    double trackAmRate = 0.0, trackAmDepth = 0.0;
    double linearTime = 0.0;
    int activeTrack = -1;
    int activeBus = -1;
    bool inBus = false;
    bool inMaster = false;
    struct PatternFrame {
        std::string name;
        std::string trackId;
        double startBeat = 0.0;
        int sourceLine = 0;
    };
    std::vector<PatternFrame> patternStack;

    Track globalTrack;
    globalTrack.id = "__global";
    globalTrack.name = "Global";
    project.tracks.push_back(globalTrack);

    struct LoopFrame {
        std::vector<NoteEvent> body;
        int count = 0;
        double startTime = 0.0;
    };
    std::vector<LoopFrame> loopStack;

    auto addEvent = [&](NoteEvent ev) {
        if (inTrack) {
            ev.decay = trackDecay;
            ev.sustain = trackSustain;
            ev.oscillators = trackOscillators;
            ev.unisonVoices = trackUnisonVoices;
            ev.unisonDetuneCents = trackUnisonDetune;
            ev.fmRatio = trackFmRatio;
            ev.fmAmount = trackFmAmount;
            ev.amRate = trackAmRate;
            ev.amDepth = trackAmDepth;
        }
        ev.trackId = activeTrack >= 0
            ? project.tracks[static_cast<size_t>(activeTrack)].id
            : globalTrack.id;
        ev.sourceLine = lineNum;
        if (!loopStack.empty())
            loopStack.back().body.push_back(ev);
        else
            events.push_back(ev);
    };

    auto readText = [](std::istringstream& stream) {
        std::string value;
        std::getline(stream, value);
        value = trim(value);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        return value;
    };

    while (std::getline(iss, line)) {
        ++lineNum;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t cpos = std::string::npos;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '#' && (i == 0 || std::isspace(static_cast<unsigned char>(line[i - 1])))) {
                cpos = i;
                break;
            }
        }
        if (cpos != std::string::npos)
            line = trim(line.substr(0, cpos));
        if (line.empty()) continue;

        std::istringstream ls(line);
        std::string cmd;
        ls >> cmd;
        cmd = toLower(cmd);

        try {
            if (cmd == "tempo") {
                double t;
                if (!(ls >> t) || t <= 0.0) throw std::runtime_error("tempo must be > 0");
                std::string at;
                double beat = 0.0;
                if (ls >> at) {
                    if (toLower(at) != "at" || !(ls >> beat) || beat < 0.0)
                        throw std::runtime_error("usage: tempo <bpm> [at <beat>]");
                    if (beat == 0.0) config.tempo = t;
                } else {
                    config.tempo = t;
                    project.tempoMap.erase(std::remove_if(project.tempoMap.begin(), project.tempoMap.end(),
                        [](const TempoPoint& point) { return point.beat == 0.0; }), project.tempoMap.end());
                }
                project.tempoMap.push_back(TempoPoint{beat, t, lineNum});
            }
            else if (cmd == "time") {
                std::string signature;
                if (!(ls >> signature)) throw std::runtime_error("usage: time <numerator>/<denominator>");
                size_t slash = signature.find('/');
                if (slash == std::string::npos) throw std::runtime_error("usage: time <numerator>/<denominator>");
                int numerator = std::stoi(signature.substr(0, slash));
                int denominator = std::stoi(signature.substr(slash + 1));
                if (numerator <= 0 || (denominator != 2 && denominator != 4 && denominator != 8 && denominator != 16))
                    throw std::runtime_error("unsupported time signature: " + signature);
                config.timeNumerator = numerator;
                config.timeDenominator = denominator;
            }
            else if (cmd == "key") {
                std::string root, mode = "major";
                if (!(ls >> root)) throw std::runtime_error("usage: key <root> [major|minor]");
                ls >> mode;
                mode = toLower(mode);
                if (mode != "major" && mode != "minor") throw std::runtime_error("key mode must be major or minor");
                config.keyRoot = pitchClass(root);
                config.keyMinor = mode == "minor";
            }
            else if (cmd == "sound" || cmd == "style" || cmd == "quality") {
                std::string mode;
                if (!(ls >> mode)) throw std::runtime_error("sound requires a mode name");
                config.soundMode = parseSoundMode(mode);
            }
            else if (cmd == "reverb") {
                double amount;
                if (!(ls >> amount) || amount < 0.0 || amount > 100.0)
                    throw std::runtime_error("reverb must be 0-100");
                config.reverb = amount / 100.0;
            }
            else if (cmd == "delay" || cmd == "echo") {
                double beats, amount;
                if (!(ls >> beats >> amount) || beats <= 0.0 || amount < 0.0 || amount > 100.0)
                    throw std::runtime_error("usage: delay <beats> <0-100>");
                config.delayBeats = beats;
                config.delayMix = amount / 100.0;
            }
            else if (cmd == "master") {
                std::string preset;
                if (!(ls >> preset)) throw std::runtime_error("master requires streaming, cd, or hires");
                preset = toLower(preset);
                if (preset == "{") {
                    if (inTrack || inBus || inMaster) throw std::runtime_error("routing blocks cannot be nested");
                    inMaster = true;
                } else if (preset == "streaming") {
                    config.sampleRate = 48000; config.bits = 24; config.channels = 2; config.masterPeakDb = -1.0;
                } else if (preset == "cd") {
                    config.sampleRate = 44100; config.bits = 16; config.channels = 2; config.masterPeakDb = -1.0;
                } else if (preset == "hires" || preset == "hi_res") {
                    config.sampleRate = 96000; config.bits = 24; config.channels = 2; config.masterPeakDb = -1.0;
                } else throw std::runtime_error("master requires streaming, cd, or hires");
            }
            else if (cmd == "samplerate") {
                int rate;
                if (!(ls >> rate) || (rate != 44100 && rate != 48000 && rate != 96000))
                    throw std::runtime_error("samplerate must be 44100, 48000, or 96000");
                config.sampleRate = rate;
            }
            else if (cmd == "bitdepth") {
                int bits;
                if (!(ls >> bits) || (bits != 8 && bits != 16 && bits != 24))
                    throw std::runtime_error("bitdepth must be 8, 16, or 24");
                config.bits = bits;
            }
            else if (cmd == "channels") {
                int channels;
                if (!(ls >> channels) || (channels != 1 && channels != 2))
                    throw std::runtime_error("channels must be 1 or 2");
                config.channels = channels;
            }
            else if (cmd == "peak") {
                double peak;
                if (!(ls >> peak) || peak > 0.0 || peak < -12.0)
                    throw std::runtime_error("peak must be between -12 and 0 dB");
                config.masterPeakDb = peak;
            }
            else if (cmd == "fadein" || cmd == "fadeout") {
                double beats;
                if (!(ls >> beats) || beats < 0.0) throw std::runtime_error(cmd + " must be >= 0 beats");
                if (cmd == "fadein") config.fadeInBeats = beats;
                else config.fadeOutBeats = beats;
            }
            else if (cmd == "title" || cmd == "artist" || cmd == "album") {
                std::string value = readText(ls);
                if (value.empty()) throw std::runtime_error(cmd + " requires text");
                if (cmd == "title") config.title = value;
                else if (cmd == "artist") config.artist = value;
                else config.album = value;
            }
            else if (cmd == "volume") {
                double v;
                if (!(ls >> v) || v < 0.0 || v > 100.0) throw std::runtime_error("volume must be 0-100");
                if (inTrack) trackVol = v / 100.0;
                else config.volume = v / 100.0;
            }
            else if (cmd == "attack" || cmd == "release" || cmd == "cutoff" || cmd == "drive") {
                if (!inTrack) throw std::runtime_error(cmd + " can only be used inside a track or instrument object");
                double value;
                if (!(ls >> value) || value < 0.0) throw std::runtime_error(cmd + " must be >= 0");
                if (cmd == "attack") trackAttack = value;
                else if (cmd == "release") trackRelease = value;
                else if (cmd == "cutoff") trackCutoff = value;
                else {
                    if (value > 100.0) throw std::runtime_error("drive must be 0-100");
                    trackDrive = value / 100.0;
                }
            }
            else if (cmd == "adsr") {
                if (!inTrack) throw std::runtime_error("adsr can only be used inside a track");
                double attack, decay, sustain, release;
                if (!(ls >> attack >> decay >> sustain >> release) || attack < 0.0 || decay < 0.0 ||
                    sustain < 0.0 || sustain > 100.0 || release < 0.0)
                    throw std::runtime_error("usage: adsr <attack-sec> <decay-sec> <sustain-0-100> <release-sec>");
                trackAttack = attack; trackDecay = decay;
                trackSustain = sustain / 100.0; trackRelease = release;
            }
            else if (cmd == "osc" || cmd == "oscillator") {
                if (!inTrack) throw std::runtime_error("osc can only be used inside a track");
                std::string waveName;
                if (!(ls >> waveName)) throw std::runtime_error("usage: osc <wave> [level=.. detune=.. semitones=..]");
                NoteEvent::Oscillator oscillator;
                oscillator.wave = parseWave(waveName);
                trackInstrument = InstrumentType::Wave;
                if (trackOscillators.empty()) trackWave = oscillator.wave;
                std::string option;
                while (ls >> option) {
                    const size_t equal = option.find('=');
                    if (equal == std::string::npos) throw std::runtime_error("osc parameters use name=value");
                    const std::string name = toLower(option.substr(0, equal));
                    const double value = std::stod(option.substr(equal + 1));
                    if (name == "level") oscillator.level = value / 100.0;
                    else if (name == "detune") oscillator.detuneCents = value;
                    else if (name == "semitones") oscillator.semitones = value;
                    else throw std::runtime_error("unknown oscillator parameter: " + name);
                }
                if (oscillator.level < 0.0) throw std::runtime_error("osc level must be >= 0");
                trackOscillators.push_back(oscillator);
            }
            else if (cmd == "unison") {
                if (!inTrack) throw std::runtime_error("unison can only be used inside a track");
                if (!(ls >> trackUnisonVoices) || (trackUnisonVoices != 1 && trackUnisonVoices != 2 &&
                    trackUnisonVoices != 4 && trackUnisonVoices != 8 && trackUnisonVoices != 16))
                    throw std::runtime_error("unison voices must be 1, 2, 4, 8, or 16");
                std::string option;
                if (ls >> option) {
                    const size_t equal = option.find('=');
                    if (equal == std::string::npos || toLower(option.substr(0, equal)) != "detune")
                        throw std::runtime_error("unison option must be detune=<cents>");
                    trackUnisonDetune = std::stod(option.substr(equal + 1));
                }
            }
            else if (cmd == "fm" || cmd == "am") {
                if (!inTrack) throw std::runtime_error(cmd + " can only be used inside a track");
                double first, second;
                if (!(ls >> first >> second) || first < 0.0 || second < 0.0)
                    throw std::runtime_error("usage: " + cmd + " <ratio-or-rate> <amount-or-depth>");
                if (cmd == "fm") { trackFmRatio = first; trackFmAmount = second; }
                else { trackAmRate = first; trackAmDepth = std::clamp(second / 100.0, 0.0, 1.0); }
            }
            else if (cmd == "wave") {
                std::string w;
                if (!(ls >> w)) throw std::runtime_error("wave requires a type");
                WaveType ww = parseWave(w);
                if (inTrack) { trackWave = ww; trackInstrument = InstrumentType::Wave; }
                else { config.wave = ww; config.instrument = InstrumentType::Wave; }
            }
            else if (cmd == "instrument") {
                std::string name;
                if (!(ls >> name)) throw std::runtime_error("instrument requires a name");
                InstrumentType inst = parseInstrument(name);
                WaveType wave = WaveType::Square;
                if (inst == InstrumentType::Wave) wave = parseWave(name);
                if (inTrack) { trackInstrument = inst; if (inst == InstrumentType::Wave) trackWave = wave; }
                else { config.instrument = inst; if (inst == InstrumentType::Wave) config.wave = wave; }
            }
            else if (cmd == "drumkit") {
                std::string name;
                if (!(ls >> name)) throw std::runtime_error("drumkit requires a name");
                DrumKit kit = parseDrumKit(name);
                if (inTrack) trackDrumKit = kit;
                else config.drumKit = kit;
            }
            else if (cmd == "pan") {
                double pan;
                if (!(ls >> pan) || pan < -100.0 || pan > 100.0)
                    throw std::runtime_error("pan must be -100 to 100");
                if (!inTrack) throw std::runtime_error("pan can only be used inside a track");
                trackPan = pan / 100.0;
            }
            else if (cmd == "mute" || cmd == "solo") {
                if (!inTrack || activeTrack < 0)
                    throw std::runtime_error(cmd + " can only be used inside a track");
                std::string value;
                if (!(ls >> value)) value = "true";
                value = toLower(value);
                if (value != "true" && value != "false" && value != "on" && value != "off")
                    throw std::runtime_error(cmd + " expects true/false or on/off");
                const bool enabled = value == "true" || value == "on";
                if (cmd == "mute") project.tracks[static_cast<size_t>(activeTrack)].mixer.mute = enabled;
                else project.tracks[static_cast<size_t>(activeTrack)].mixer.solo = enabled;
            }
            else if (cmd == "gain") {
                double db;
                if (!(ls >> db) || !std::isfinite(db) || db < -96.0 || db > 24.0)
                    throw std::runtime_error("gain must be between -96 and 24 dB");
                if (inTrack && activeTrack >= 0)
                    project.tracks[static_cast<size_t>(activeTrack)].mixer.gain = std::pow(10.0, db / 20.0);
                else if (inBus && activeBus >= 0)
                    project.buses[static_cast<size_t>(activeBus)].mixer.gain = std::pow(10.0, db / 20.0);
                else throw std::runtime_error("gain can only be used inside a track or bus");
            }
            else if (cmd == "fx") {
                std::string typeName;
                if (!(ls >> typeName)) throw std::runtime_error("usage: fx <type> [parameter=value ...]");
                Effect effect;
                effect.type = parseEffectType(typeName);
                std::string option;
                while (ls >> option) {
                    const size_t equal = option.find('=');
                    if (equal == std::string::npos || equal == 0 || equal + 1 >= option.size())
                        throw std::runtime_error("effect parameters use name=value");
                    const std::string name = toLower(option.substr(0, equal));
                    const double value = std::stod(option.substr(equal + 1));
                    if (!std::isfinite(value)) throw std::runtime_error("effect parameter must be finite");
                    if (name == "wet") {
                        if (value < 0.0 || value > 100.0) throw std::runtime_error("wet must be 0-100");
                        effect.wet = value / 100.0;
                    } else effect.parameters[name] = value;
                }
                if (inTrack && activeTrack >= 0)
                    project.tracks[static_cast<size_t>(activeTrack)].inserts.push_back(effect);
                else if (inBus && activeBus >= 0)
                    project.buses[static_cast<size_t>(activeBus)].inserts.push_back(effect);
                else if (inMaster) project.master.inserts.push_back(effect);
                else throw std::runtime_error("fx can only be used inside track, bus, or master");
            }
            else if (cmd == "send") {
                if (!inTrack || activeTrack < 0) throw std::runtime_error("send can only be used inside a track");
                std::string bus;
                double amount;
                if (!(ls >> bus >> amount) || amount < 0.0 || amount > 100.0)
                    throw std::runtime_error("usage: send <bus> <0-100>");
                project.tracks[static_cast<size_t>(activeTrack)].sends.push_back(Send{bus, amount / 100.0});
            }
            else if (cmd == "sidechain") {
                if (!inTrack || activeTrack < 0) throw std::runtime_error("sidechain can only be used inside a track");
                Sidechain sidechain;
                if (!(ls >> sidechain.sourceTrackId))
                    throw std::runtime_error("usage: sidechain <source-track> [name=value ...]");
                std::string option;
                while (ls >> option) {
                    const size_t equal = option.find('=');
                    if (equal == std::string::npos || equal == 0 || equal + 1 >= option.size())
                        throw std::runtime_error("sidechain parameters use name=value");
                    const std::string name = toLower(option.substr(0, equal));
                    const double value = std::stod(option.substr(equal + 1));
                    if (!std::isfinite(value)) throw std::runtime_error("sidechain parameter must be finite");
                    if (name == "amount") sidechain.amount = value / 100.0;
                    else if (name == "threshold") sidechain.thresholdDb = value;
                    else if (name == "ratio") sidechain.ratio = value;
                    else if (name == "attack") sidechain.attackMs = value;
                    else if (name == "release") sidechain.releaseMs = value;
                    else throw std::runtime_error("unknown sidechain parameter: " + name);
                }
                project.tracks[static_cast<size_t>(activeTrack)].sidechains.push_back(sidechain);
            }
            else if (cmd == "automate") {
                if (!inTrack || activeTrack < 0)
                    throw std::runtime_error("automate can only be used inside a track");
                std::string parameter, at, curveName = "linear";
                double beat, value;
                if (!(ls >> parameter >> at >> beat >> value) || toLower(at) != "at" || beat < 0.0)
                    throw std::runtime_error("usage: automate <volume|pan|cutoff|drive> at <beat> <value> [linear|step]");
                parameter = toLower(parameter);
                if (parameter != "volume" && parameter != "pan" && parameter != "cutoff" && parameter != "drive")
                    throw std::runtime_error("automation parameter must be volume, pan, cutoff, or drive");
                if ((parameter == "volume" || parameter == "drive") && (value < 0.0 || value > 100.0))
                    throw std::runtime_error(parameter + " automation must be 0-100");
                if (parameter == "pan" && (value < -100.0 || value > 100.0))
                    throw std::runtime_error("pan automation must be -100 to 100");
                if (parameter == "cutoff" && value < 0.0)
                    throw std::runtime_error("cutoff automation must be >= 0");
                if (ls >> curveName) curveName = toLower(curveName);
                if (curveName != "linear" && curveName != "step")
                    throw std::runtime_error("automation curve must be linear or step");
                const std::string& trackId = project.tracks[static_cast<size_t>(activeTrack)].id;
                auto lane = std::find_if(project.automation.begin(), project.automation.end(),
                    [&](const AutomationLane& item) { return item.trackId == trackId && item.parameter == parameter; });
                if (lane == project.automation.end()) {
                    project.automation.push_back(AutomationLane{trackId, parameter, {}});
                    lane = std::prev(project.automation.end());
                }
                if (!lane->points.empty() && beat < lane->points.back().beat)
                    throw std::runtime_error("automation points must be written in beat order");
                lane->points.push_back(AutomationPoint{beat, value,
                    curveName == "step" ? AutomationCurve::Step : AutomationCurve::Linear, lineNum});
            }
            else if (cmd == "marker") {
                std::string name;
                double beat = inTrack ? trackTime : linearTime;
                if (!(ls >> name)) throw std::runtime_error("usage: marker <name> [beat]");
                if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
                    name = name.substr(1, name.size() - 2);
                if (ls >> beat) {
                    if (beat < 0.0) throw std::runtime_error("marker beat must be >= 0");
                }
                project.markers.push_back(Marker{name, beat, lineNum});
            }
            else if (cmd == "grid") {
                std::string division, option;
                if (!(ls >> division)) throw std::runtime_error("usage: grid <1/4|1/8|1/16|1/32> [triplet]");
                const size_t slash = division.find('/');
                if (slash == std::string::npos || division.substr(0, slash) != "1")
                    throw std::runtime_error("grid must be written as 1/<division>");
                const int denominator = std::stoi(division.substr(slash + 1));
                if (denominator < 1 || denominator > 128 || (denominator & (denominator - 1)) != 0)
                    throw std::runtime_error("grid division must be a power of two from 1 to 128");
                project.timeline.gridBeats = 4.0 / denominator;
                if (ls >> option) {
                    if (toLower(option) != "triplet") throw std::runtime_error("grid option must be triplet");
                    project.timeline.gridBeats *= 2.0 / 3.0;
                }
            }
            else if (cmd == "swing") {
                double percent;
                if (!(ls >> percent) || percent < 50.0 || percent > 75.0)
                    throw std::runtime_error("swing must be between 50 (straight) and 75 percent");
                project.timeline.swingPercent = percent;
            }
            else if (cmd == "section") {
                std::string name, at, length;
                double startBeat, lengthBeats;
                if (!(ls >> name >> at >> startBeat >> length >> lengthBeats) ||
                    toLower(at) != "at" || toLower(length) != "length" ||
                    startBeat < 0.0 || lengthBeats <= 0.0)
                    throw std::runtime_error("usage: section <name> at <beat> length <beats>");
                project.sections.push_back(Section{name, startBeat, lengthBeats, lineNum});
                project.markers.push_back(Marker{name, startBeat, lineNum});
            }
            else if (cmd == "bus") {
                if (inTrack || inBus || inMaster) throw std::runtime_error("routing blocks cannot be nested");
                std::string name, brace;
                if (!(ls >> name >> brace) || brace != "{") throw std::runtime_error("usage: bus <name> {");
                auto duplicate = std::find_if(project.buses.begin(), project.buses.end(),
                    [&](const Bus& bus) { return bus.id == name; });
                if (duplicate != project.buses.end()) throw std::runtime_error("duplicate bus name: " + name);
                project.buses.push_back(Bus{name, {}, {}});
                activeBus = static_cast<int>(project.buses.size() - 1);
                inBus = true;
            }
            else if (cmd == "track") {
                if (inTrack || inBus || inMaster) throw std::runtime_error("routing blocks cannot be nested");
                std::string name;
                if (!(ls >> name)) throw std::runtime_error("usage: track <name> {");
                if (name == "{") throw std::runtime_error("track requires a name");
                auto duplicate = std::find_if(project.tracks.begin(), project.tracks.end(),
                    [&](const Track& item) { return item.id == name; });
                if (duplicate != project.tracks.end())
                    throw std::runtime_error("duplicate track name: " + name);
                Track track;
                track.id = name;
                track.name = name;
                project.tracks.push_back(track);
                activeTrack = static_cast<int>(project.tracks.size() - 1);
                inTrack = true;
                trackTime = 0.0;
                trackWave = config.wave;
                trackInstrument = config.instrument;
                trackDrumKit = config.drumKit;
                trackPan = 0.0;
                trackAttack = -1.0;
                trackDecay = -1.0;
                trackSustain = -1.0;
                trackRelease = -1.0;
                trackCutoff = 0.0;
                trackDrive = 0.0;
                trackVol = config.volume;
                trackOscillators.clear();
                trackUnisonVoices = 1; trackUnisonDetune = 0.0;
                trackFmRatio = 0.0; trackFmAmount = 0.0;
                trackAmRate = 0.0; trackAmDepth = 0.0;
            }
            else if (cmd == "endtrack" || (cmd == "}" && inTrack && loopStack.empty())) {
                inTrack = false;
                activeTrack = -1;
            }
            else if (cmd == "note") {
                std::string name;
                double beats;
                if (!(ls >> name >> beats) || beats <= 0.0)
                    throw std::runtime_error("usage: note <pitch> <beats>");
                NoteEvent ev;
                ev.freqs = { noteToFreq(name) };
                ev.durationBeats = beats;
                ev.volume = inTrack ? trackVol : config.volume;
                ev.wave = inTrack ? trackWave : config.wave;
                ev.instrument = inTrack ? trackInstrument : config.instrument;
                ev.pan = inTrack ? trackPan : 0.0;
                ev.attack = inTrack ? trackAttack : -1.0;
                ev.release = inTrack ? trackRelease : -1.0;
                ev.cutoff = inTrack ? trackCutoff : 0.0;
                ev.drive = inTrack ? trackDrive : 0.0;
                std::string option;
                if (ls >> option) {
                    double velocity;
                    if (toLower(option) != "velocity" || !(ls >> velocity) || velocity < 0.0 || velocity > 100.0)
                        throw std::runtime_error("note option must be: velocity <0-100>");
                    ev.volume *= velocity / 100.0;
                }
                ev.startBeat = inTrack ? trackTime : linearTime;
                addEvent(ev);
                if (inTrack) trackTime += beats;
                else linearTime += beats;
            }
            else if (cmd == "rest") {
                double beats;
                if (!(ls >> beats) || beats <= 0.0)
                    throw std::runtime_error("usage: rest <beats>");
                if (inTrack) trackTime += beats;
                else linearTime += beats;
            }
            else if (cmd == "at") {
                double beat;
                if (!(ls >> beat) || beat < 0.0) throw std::runtime_error("usage: at <beat>");
                if (inTrack) trackTime = beat;
                else linearTime = beat;
            }
            else if (cmd == "patternbegin") {
                std::string name;
                if (!(ls >> name)) throw std::runtime_error("patternbegin requires a name");
                const std::string trackId = activeTrack >= 0
                    ? project.tracks[static_cast<size_t>(activeTrack)].id : globalTrack.id;
                patternStack.push_back(PatternFrame{name, trackId,
                    inTrack ? trackTime : linearTime, lineNum});
            }
            else if (cmd == "patternend") {
                if (patternStack.empty()) throw std::runtime_error("patternend without patternbegin");
                const auto frame = patternStack.back();
                patternStack.pop_back();
                const double endBeat = inTrack ? trackTime : linearTime;
                project.arrangement.push_back(PatternPlacement{frame.name, frame.trackId,
                    frame.startBeat, std::max(0.0, endBeat - frame.startBeat), frame.sourceLine});
            }
            else if (cmd == "degree") {
                int degree, octave;
                double beats;
                if (!(ls >> degree >> octave >> beats) || degree == 0 || octave < 0 || octave > 8 || beats <= 0.0)
                    throw std::runtime_error("usage: degree <non-zero degree> <octave> <beats>");
                static const int majorScale[] = {0,2,4,5,7,9,11};
                static const int minorScale[] = {0,2,3,5,7,8,10};
                int zeroBased = degree > 0 ? degree - 1 : degree;
                int scaleIndex = ((zeroBased % 7) + 7) % 7;
                int octaveShift = static_cast<int>(std::floor(zeroBased / 7.0));
                int semitone = (config.keyMinor ? minorScale[scaleIndex] : majorScale[scaleIndex]);
                int midi = (octave + 1 + octaveShift) * 12 + config.keyRoot + semitone;
                NoteEvent ev;
                ev.freqs = {440.0 * std::pow(2.0, (midi - 69) / 12.0)};
                ev.durationBeats = beats;
                ev.volume = inTrack ? trackVol : config.volume;
                ev.wave = inTrack ? trackWave : config.wave;
                ev.instrument = inTrack ? trackInstrument : config.instrument;
                ev.pan = inTrack ? trackPan : 0.0;
                ev.attack = inTrack ? trackAttack : -1.0;
                ev.release = inTrack ? trackRelease : -1.0;
                ev.cutoff = inTrack ? trackCutoff : 0.0;
                ev.drive = inTrack ? trackDrive : 0.0;
                ev.startBeat = inTrack ? trackTime : linearTime;
                addEvent(ev);
                if (inTrack) trackTime += beats; else linearTime += beats;
            }
            else if (cmd == "chord") {
                std::vector<std::string> notes;
                std::string tok;
                double beats = 0.0;
                while (ls >> tok) {
                    try {
                        size_t idx;
                        double val = std::stod(tok, &idx);
                        if (idx == tok.size()) { beats = val; break; }
                    } catch (...) {}
                    notes.push_back(tok);
                }
                if (notes.empty() || beats <= 0.0)
                    throw std::runtime_error("usage: chord <notes...> <beats>");
                NoteEvent ev;
                for (const auto& n : notes) ev.freqs.push_back(noteToFreq(n));
                ev.durationBeats = beats;
                ev.volume = inTrack ? trackVol : config.volume;
                ev.wave = inTrack ? trackWave : config.wave;
                ev.instrument = inTrack ? trackInstrument : config.instrument;
                ev.pan = inTrack ? trackPan : 0.0;
                ev.attack = inTrack ? trackAttack : -1.0;
                ev.release = inTrack ? trackRelease : -1.0;
                ev.cutoff = inTrack ? trackCutoff : 0.0;
                ev.drive = inTrack ? trackDrive : 0.0;
                std::string option;
                if (ls >> option) {
                    double velocity;
                    if (toLower(option) != "velocity" || !(ls >> velocity) || velocity < 0.0 || velocity > 100.0)
                        throw std::runtime_error("chord option must be: velocity <0-100>");
                    ev.volume *= velocity / 100.0;
                }
                ev.startBeat = inTrack ? trackTime : linearTime;
                addEvent(ev);
                if (inTrack) trackTime += beats;
                else linearTime += beats;
            }
            else if (cmd == "harmony" || cmd == "chordsym") {
                std::string symbol;
                int octave;
                double beats;
                if (!(ls >> symbol >> octave >> beats) || octave < 0 || octave > 8 || beats <= 0.0)
                    throw std::runtime_error("usage: harmony <symbol> <octave> <beats>");
                NoteEvent ev;
                ev.freqs = chordSymbolToFreqs(symbol, octave);
                ev.durationBeats = beats;
                ev.volume = inTrack ? trackVol : config.volume;
                ev.wave = inTrack ? trackWave : config.wave;
                ev.instrument = inTrack ? trackInstrument : config.instrument;
                ev.pan = inTrack ? trackPan : 0.0;
                ev.attack = inTrack ? trackAttack : -1.0;
                ev.release = inTrack ? trackRelease : -1.0;
                ev.cutoff = inTrack ? trackCutoff : 0.0;
                ev.drive = inTrack ? trackDrive : 0.0;
                ev.startBeat = inTrack ? trackTime : linearTime;
                addEvent(ev);
                if (inTrack) trackTime += beats; else linearTime += beats;
            }
            else if (cmd == "kick" || cmd == "snare" || cmd == "hihat" || cmd == "hat" ||
                     cmd == "openhat" || cmd == "open_hihat" || cmd == "tom" ||
                     cmd == "low_tom" || cmd == "high_tom" || cmd == "clap" ||
                     cmd == "rimshot" || cmd == "rim" || cmd == "crash" || cmd == "ride" ||
                     cmd == "cowbell" || cmd == "shaker" || cmd == "tambourine" ||
                     cmd == "timpani" || cmd == "impact" || cmd == "drum") {
                std::string drumName = cmd;
                if (cmd == "drum" && !(ls >> drumName))
                    throw std::runtime_error("usage: drum <name> [beats]");
                double beats = 0.25;
                ls >> beats;
                if (beats <= 0.0) throw std::runtime_error("drum duration must be > 0");

                NoteEvent ev;
                ev.durationBeats = beats;
                ev.volume = inTrack ? trackVol : config.volume;
                ev.startBeat = inTrack ? trackTime : linearTime;
                ev.drum = parseDrum(drumName);
                ev.drumKit = inTrack ? trackDrumKit : config.drumKit;
                ev.pan = inTrack ? trackPan : 0.0;
                ev.attack = inTrack ? trackAttack : -1.0;
                ev.release = inTrack ? trackRelease : -1.0;
                ev.cutoff = inTrack ? trackCutoff : 0.0;
                ev.drive = inTrack ? trackDrive : 0.0;
                std::string drumOption;
                if (ls >> drumOption) {
                    double velocity;
                    if (toLower(drumOption) != "velocity" || !(ls >> velocity) || velocity < 0.0 || velocity > 100.0)
                        throw std::runtime_error("drum option must be: velocity <0-100>");
                    ev.volume *= velocity / 100.0;
                }
                addEvent(ev);
                if (inTrack) trackTime += beats;
                else linearTime += beats;
            }
            else if (cmd == "loop") {
                int count;
                std::string brace;
                if (!(ls >> count >> brace) || count <= 0 || brace != "{")
                    throw std::runtime_error("usage: loop <N> {");
                LoopFrame frame;
                frame.count = count;
                frame.startTime = inTrack ? trackTime : linearTime;
                loopStack.push_back(frame);
            }
            else if (cmd == "}") {
                if (loopStack.empty()) {
                    if (inTrack) { inTrack = false; activeTrack = -1; continue; }
                    if (inBus) { inBus = false; activeBus = -1; continue; }
                    if (inMaster) { inMaster = false; continue; }
                    throw std::runtime_error("Unexpected '}'");
                }

                auto body = loopStack.back().body;
                int count = loopStack.back().count;
                double startT = loopStack.back().startTime;
                loopStack.pop_back();

                double bodyDur = 0.0;
                for (const auto& e : body)
                    bodyDur = std::max(bodyDur, (e.startBeat - startT) + e.durationBeats);

                std::vector<NoteEvent> repeated;
                for (int i = 0; i < count; ++i) {
                    for (auto e : body) {
                        e.startBeat = startT + i * bodyDur + (e.startBeat - startT);
                        repeated.push_back(e);
                    }
                }

                if (loopStack.empty())
                    events.insert(events.end(), repeated.begin(), repeated.end());
                else
                    loopStack.back().body.insert(loopStack.back().body.end(),
                                                 repeated.begin(), repeated.end());

                if (inTrack) trackTime = startT + count * bodyDur;
                else linearTime = startT + count * bodyDur;
            }
            else {
                throw std::runtime_error("Unknown command: " + cmd);
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Line " + std::to_string(lineNum) + ": " + e.what());
        }
    }

    if (!loopStack.empty())
        throw std::runtime_error("Unclosed loop {");
    if (!patternStack.empty())
        throw std::runtime_error("Unclosed pattern placement");
    if (inTrack)
        throw std::runtime_error("Unclosed track {");
    if (inBus || inMaster)
        throw std::runtime_error("Unclosed routing block {");

    for (const auto& event : events) {
        auto track = std::find_if(project.tracks.begin(), project.tracks.end(),
            [&](const Track& item) { return item.id == event.trackId; });
        if (track == project.tracks.end())
            throw std::runtime_error("Internal error: event targets unknown track " + event.trackId);
        track->events.push_back(event);
    }
    if (project.tracks.front().events.empty()) project.tracks.erase(project.tracks.begin());
    project.config = config;
    std::stable_sort(project.tempoMap.begin(), project.tempoMap.end(),
        [](const TempoPoint& a, const TempoPoint& b) { return a.beat < b.beat; });
    if (project.tempoMap.empty() || project.tempoMap.front().beat > 0.0)
        project.tempoMap.insert(project.tempoMap.begin(), TempoPoint{0.0, config.tempo, 0});
    project.validate();
    events = project.renderEvents();
}

} // namespace lyra
