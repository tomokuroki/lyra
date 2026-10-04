#include "export.hpp"
#include <fstream>
#include <cmath>

namespace lyra {

void writeWav(const std::string& filename, const std::vector<int16_t>& samples, const Config& cfg) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot create file: " + filename);

    uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    uint32_t fileSize = 36 + dataSize;
    uint16_t channels = 1;
    uint16_t bits = 16;
    uint32_t rate = static_cast<uint32_t>(cfg.sampleRate);

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&fileSize), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    uint32_t fmtSize = 16;
    out.write(reinterpret_cast<const char*>(&fmtSize), 4);
    uint16_t audioFormat = 1;
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&channels), 2);
    out.write(reinterpret_cast<const char*>(&rate), 4);
    uint32_t byteRate = rate * channels * (bits / 8);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = channels * (bits / 8);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bits), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
    out.write(reinterpret_cast<const char*>(samples.data()), dataSize);
}

void writeMidi(const std::string& filename, const std::vector<NoteEvent>& events, const Config& cfg) {
    const int TPQ = 480;
    std::vector<uint8_t> track;

    auto writeVar = [&](uint32_t value) {
        uint8_t buf[4];
        int n = 0;
        buf[n++] = value & 0x7F;
        while (value >>= 7) buf[n++] = 0x80 | (value & 0x7F);
        for (int i = n - 1; i >= 0; --i) track.push_back(buf[i]);
    };

    uint32_t usPerBeat = static_cast<uint32_t>(60000000.0 / cfg.tempo);
    track.push_back(0x00);
    track.push_back(0xFF);
    track.push_back(0x51);
    track.push_back(0x03);
    track.push_back((usPerBeat >> 16) & 0xFF);
    track.push_back((usPerBeat >> 8) & 0xFF);
    track.push_back(usPerBeat & 0xFF);

    track.push_back(0x00);
    track.push_back(0xC0);
    track.push_back(80);

    std::vector<NoteEvent> sorted = events;
    std::sort(sorted.begin(), sorted.end(),
              [](const NoteEvent& a, const NoteEvent& b) { return a.startBeat < b.startBeat; });

    double lastBeat = 0.0;
    for (const auto& ev : sorted) {
        if (ev.drum != DrumType::None) continue;
        if (ev.freqs.empty() || ev.freqs[0] <= 0.0) continue;

        uint32_t delta = static_cast<uint32_t>((ev.startBeat - lastBeat) * TPQ + 0.5);
        lastBeat = ev.startBeat;

        bool first = true;
        for (double f : ev.freqs) {
            if (f <= 0.0) continue;
            int note = std::max(0, std::min(127, static_cast<int>(std::round(69.0 + 12.0 * std::log2(f / 440.0)))));
            int vel = std::max(1, std::min(127, static_cast<int>(ev.volume * 100)));
            if (first) { writeVar(delta); first = false; }
            else writeVar(0);
            track.push_back(0x90);
            track.push_back(static_cast<uint8_t>(note));
            track.push_back(static_cast<uint8_t>(vel));
        }

        uint32_t dur = static_cast<uint32_t>(ev.durationBeats * TPQ + 0.5);
        first = true;
        for (double f : ev.freqs) {
            if (f <= 0.0) continue;
            int note = std::max(0, std::min(127, static_cast<int>(std::round(69.0 + 12.0 * std::log2(f / 440.0)))));
            if (first) { writeVar(dur); first = false; }
            else writeVar(0);
            track.push_back(0x80);
            track.push_back(static_cast<uint8_t>(note));
            track.push_back(0);
        }
        lastBeat += ev.durationBeats;
    }

    track.push_back(0x00);
    track.push_back(0xFF);
    track.push_back(0x2F);
    track.push_back(0x00);

    std::ofstream out(filename, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot create MIDI file: " + filename);

    auto writeBE32 = [&](uint32_t v) {
        uint8_t b[4] = { uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v) };
        out.write(reinterpret_cast<char*>(b), 4);
    };
    auto writeBE16 = [&](uint16_t v) {
        uint8_t b[2] = { uint8_t(v >> 8), uint8_t(v) };
        out.write(reinterpret_cast<char*>(b), 2);
    };

    out.write("MThd", 4);
    writeBE32(6);
    writeBE16(0);
    writeBE16(1);
    writeBE16(TPQ);
    out.write("MTrk", 4);
    writeBE32(static_cast<uint32_t>(track.size()));
    out.write(reinterpret_cast<const char*>(track.data()), track.size());
}

} // namespace lyra
