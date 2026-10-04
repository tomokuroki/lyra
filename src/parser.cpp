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
            else if (cmd == "volume") {
                double v;
                if (!(ls >> v) || v < 0.0 || v > 100.0) throw std::runtime_error("volume must be 0-100");
                if (inTrack) trackVol = v / 100.0;
                else config.volume = v / 100.0;
            }
            else if (cmd == "wave" || cmd == "instrument") {
                std::string w;
                if (!(ls >> w)) throw std::runtime_error("wave requires a type");
                WaveType ww = parseWave(w);
                if (inTrack) trackWave = ww;
                else config.wave = ww;
            }
            else if (cmd == "track") {
                inTrack = true;
                trackTime = 0.0;
                trackWave = config.wave;
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
                ev.startBeat = inTrack ? trackTime : linearTime;
                addEvent(ev);
                if (inTrack) trackTime += beats;
                else linearTime += beats;
            }
            else if (cmd == "kick" || cmd == "snare" || cmd == "hihat" || cmd == "tom") {
                double beats = 0.25;
                ls >> beats;
                if (beats <= 0.0) beats = 0.25;

                NoteEvent ev;
                ev.durationBeats = beats;
                ev.volume = inTrack ? trackVol : config.volume;
                ev.startBeat = inTrack ? trackTime : linearTime;
                if (cmd == "kick")  ev.drum = DrumType::Kick;
                if (cmd == "snare") ev.drum = DrumType::Snare;
                if (cmd == "hihat") ev.drum = DrumType::Hihat;
                if (cmd == "tom")   ev.drum = DrumType::Tom;
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
