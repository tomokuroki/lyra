#include "parser.hpp"
#include <sstream>
#include <iostream>

namespace lyra {

void Parser::parse(const std::string& source) {
    std::istringstream iss(source);
    std::string line;
    int lineNum = 0;

    double trackTime = 0.0;
    bool inTrack = false;
    WaveType trackWave = WaveType::Square;
    InstrumentType trackInstrument = InstrumentType::Wave;
    DrumKit trackDrumKit = DrumKit::Standard;
    double trackPan = 0.0;
    double trackVol = 0.7;
    double linearTime = 0.0;

    struct LoopFrame {
        std::vector<NoteEvent> body;
        int count = 0;
        double startTime = 0.0;
    };
    std::vector<LoopFrame> loopStack;

    auto addEvent = [&](NoteEvent ev) {
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
                config.tempo = t;
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
                if (preset == "streaming") {
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
            else if (cmd == "track") {
                inTrack = true;
                trackTime = 0.0;
                trackWave = config.wave;
                trackInstrument = config.instrument;
                trackDrumKit = config.drumKit;
                trackPan = 0.0;
                trackVol = config.volume;
            }
            else if (cmd == "endtrack" || (cmd == "}" && inTrack && loopStack.empty())) {
                inTrack = false;
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
                ev.startBeat = inTrack ? trackTime : linearTime;
                addEvent(ev);
                if (inTrack) trackTime += beats;
                else linearTime += beats;
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
                    if (inTrack) { inTrack = false; continue; }
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
}

} // namespace lyra
