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

    struct MidiMessage {
        uint32_t tick;
        int order;
        std::vector<uint8_t> data;
    };
    std::vector<MidiMessage> messages;

    auto melodicChannel = [](InstrumentType instrument) {
        int channel = static_cast<int>(instrument);
        return channel >= 9 ? channel + 1 : channel; // MIDI channel 10 (index 9) is percussion.
    };
    auto midiProgram = [](InstrumentType instrument) {
        switch (instrument) {
            case InstrumentType::Piano: return 0;
            case InstrumentType::ElectricPiano: return 4;
            case InstrumentType::Organ: return 19;
            case InstrumentType::MusicBox: return 10;
            case InstrumentType::Glockenspiel: return 9;
            case InstrumentType::Strings: return 48;
            case InstrumentType::Brass: return 61;
            case InstrumentType::Flute: return 73;
            case InstrumentType::Guitar: return 24;
            case InstrumentType::ElectricGuitar: return 30;
            case InstrumentType::SynthBass: return 38;
            case InstrumentType::Wave: return 80;
        }
        return 80;
    };
    auto drumNote = [](DrumType drum) {
        switch (drum) {
            case DrumType::Kick: return 36;
            case DrumType::Snare: return 38;
            case DrumType::Hihat: return 42;
            case DrumType::OpenHihat: return 46;
            case DrumType::TomLow: return 45;
            case DrumType::TomMid: return 47;
            case DrumType::TomHigh: return 50;
            case DrumType::Clap: return 39;
            case DrumType::Rimshot: return 37;
            case DrumType::Crash: return 49;
            case DrumType::Ride: return 51;
            case DrumType::Cowbell: return 56;
            case DrumType::Shaker: return 82;
            case DrumType::Tambourine: return 54;
            case DrumType::Timpani: return 47;
            case DrumType::Impact: return 55;
            case DrumType::None: return 0;
        }
        return 0;
    };

    bool programs[16] = {};
    for (const auto& ev : events) {
        uint32_t start = static_cast<uint32_t>(std::max(0.0, ev.startBeat) * TPQ + 0.5);
        uint32_t end = start + static_cast<uint32_t>(ev.durationBeats * TPQ + 0.5);
        int velocity = std::max(1, std::min(127, static_cast<int>(ev.volume * 110)));

        if (ev.drum != DrumType::None) {
            int note = drumNote(ev.drum);
            messages.push_back({start, 2, {0x99, static_cast<uint8_t>(note), static_cast<uint8_t>(velocity)}});
            messages.push_back({end, 1, {0x89, static_cast<uint8_t>(note), 0}});
            continue;
        }

        int channel = melodicChannel(ev.instrument);
        if (!programs[channel]) {
            messages.push_back({0, 0, {static_cast<uint8_t>(0xC0 | channel), static_cast<uint8_t>(midiProgram(ev.instrument))}});
            programs[channel] = true;
        }
        for (double f : ev.freqs) {
            if (f <= 0.0) continue;
            int note = std::max(0, std::min(127, static_cast<int>(std::round(69.0 + 12.0 * std::log2(f / 440.0)))));
            messages.push_back({start, 2, {static_cast<uint8_t>(0x90 | channel), static_cast<uint8_t>(note), static_cast<uint8_t>(velocity)}});
            messages.push_back({end, 1, {static_cast<uint8_t>(0x80 | channel), static_cast<uint8_t>(note), 0}});
        }
    }

    std::stable_sort(messages.begin(), messages.end(), [](const MidiMessage& a, const MidiMessage& b) {
        return a.tick < b.tick || (a.tick == b.tick && a.order < b.order);
    });
    uint32_t lastTick = 0;
    for (const auto& message : messages) {
        writeVar(message.tick - lastTick);
        track.insert(track.end(), message.data.begin(), message.data.end());
        lastTick = message.tick;
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
