#include "audio_file.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace lyra {
namespace {

uint16_t le16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1]) << 8; }
uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
uint16_t be16(const uint8_t* p) { return uint16_t(p[0]) << 8 | uint16_t(p[1]); }
uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]); }

std::vector<uint8_t> readBytes(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open audio asset: " + path);
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0);
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

SourceAudio loadWav(const std::string& path) {
    auto bytes = readBytes(path);
    if (bytes.size() < 12 || std::string(reinterpret_cast<char*>(bytes.data()), 4) != "RIFF" ||
        std::string(reinterpret_cast<char*>(bytes.data() + 8), 4) != "WAVE")
        throw std::runtime_error("Invalid WAV file: " + path);
    uint16_t format = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    const uint8_t* data = nullptr; size_t dataSize = 0;
    for (size_t pos = 12; pos + 8 <= bytes.size();) {
        const std::string id(reinterpret_cast<char*>(bytes.data() + pos), 4);
        const uint32_t size = le32(bytes.data() + pos + 4);
        const size_t body = pos + 8;
        if (body + size > bytes.size()) break;
        if (id == "fmt " && size >= 16) {
            format = le16(bytes.data() + body); channels = le16(bytes.data() + body + 2);
            rate = le32(bytes.data() + body + 4); bits = le16(bytes.data() + body + 14);
        } else if (id == "data") { data = bytes.data() + body; dataSize = size; }
        pos = body + size + (size & 1u);
    }
    if (!data || channels == 0 || rate == 0 || (format != 1 && format != 3))
        throw std::runtime_error("Unsupported WAV encoding: " + path);
    const size_t bytesPerSample = bits / 8;
    if (bytesPerSample == 0) throw std::runtime_error("Invalid WAV bit depth");
    SourceAudio audio; audio.channels = channels; audio.sampleRate = static_cast<int>(rate);
    const size_t count = dataSize / bytesPerSample; audio.samples.resize(count);
    for (size_t i = 0; i < count; ++i) {
        const uint8_t* p = data + i * bytesPerSample;
        double value = 0.0;
        if (format == 3 && bits == 32) { float f; std::memcpy(&f, p, 4); value = f; }
        else if (bits == 8) value = (static_cast<int>(p[0]) - 128) / 128.0;
        else if (bits == 16) value = static_cast<int16_t>(le16(p)) / 32768.0;
        else if (bits == 24) { int32_t v = int32_t(p[0]) | int32_t(p[1]) << 8 | int32_t(p[2]) << 16; if (v & 0x800000) v |= ~0xFFFFFF; value = v / 8388608.0; }
        else if (bits == 32) value = static_cast<int32_t>(le32(p)) / 2147483648.0;
        else throw std::runtime_error("Unsupported WAV bit depth: " + std::to_string(bits));
        audio.samples[i] = static_cast<float>(std::clamp(value, -1.0, 1.0));
    }
    return audio;
}

double extended80(const uint8_t* p) {
    const int exponent = int(be16(p) & 0x7FFF) - 16383;
    uint64_t mantissa = 0; for (int i = 0; i < 8; ++i) mantissa = (mantissa << 8) | p[2 + i];
    return std::ldexp(static_cast<double>(mantissa), exponent - 63);
}

SourceAudio loadAiff(const std::string& path) {
    auto bytes = readBytes(path);
    if (bytes.size() < 12 || std::string(reinterpret_cast<char*>(bytes.data()), 4) != "FORM" ||
        std::string(reinterpret_cast<char*>(bytes.data() + 8), 4) != "AIFF")
        throw std::runtime_error("Invalid or compressed AIFF file: " + path);
    uint16_t channels = 0, bits = 0; int rate = 0;
    const uint8_t* data = nullptr; size_t dataSize = 0;
    for (size_t pos = 12; pos + 8 <= bytes.size();) {
        const std::string id(reinterpret_cast<char*>(bytes.data() + pos), 4);
        const uint32_t size = be32(bytes.data() + pos + 4); const size_t body = pos + 8;
        if (body + size > bytes.size()) break;
        if (id == "COMM" && size >= 18) { channels = be16(bytes.data() + body); bits = be16(bytes.data() + body + 6); rate = static_cast<int>(std::lround(extended80(bytes.data() + body + 8))); }
        else if (id == "SSND" && size >= 8) { const uint32_t offset = be32(bytes.data() + body); data = bytes.data() + body + 8 + offset; dataSize = size - 8 - offset; }
        pos = body + size + (size & 1u);
    }
    if (!data || channels == 0 || rate <= 0) throw std::runtime_error("Invalid AIFF chunks: " + path);
    const size_t stride = bits / 8; SourceAudio audio; audio.channels = channels; audio.sampleRate = rate;
    audio.samples.resize(dataSize / stride);
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        const uint8_t* p = data + i * stride; double value;
        if (bits == 8) value = static_cast<int8_t>(p[0]) / 128.0;
        else if (bits == 16) value = static_cast<int16_t>(be16(p)) / 32768.0;
        else if (bits == 24) { int32_t v = int32_t(p[0]) << 16 | int32_t(p[1]) << 8 | p[2]; if (v & 0x800000) v |= ~0xFFFFFF; value = v / 8388608.0; }
        else throw std::runtime_error("Unsupported AIFF bit depth: " + std::to_string(bits));
        audio.samples[i] = static_cast<float>(value);
    }
    return audio;
}

std::string quoted(const std::string& value) { return "\"" + value + "\""; }

} // namespace

SourceAudio loadAudioFile(const std::string& path) {
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    if (extension == ".wav" || extension == ".wave") return loadWav(path);
    if (extension == ".aiff" || extension == ".aif") return loadAiff(path);
    if (extension == ".flac" || extension == ".mp3" || extension == ".ogg") {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto temporary = std::filesystem::temp_directory_path() / ("lyra-decode-" + std::to_string(stamp) + ".wav");
        const std::string command = "ffmpeg -nostdin -hide_banner -loglevel error -y -i " + quoted(path)
            + " -c:a pcm_s16le " + quoted(temporary.string());
        if (std::system(command.c_str()) != 0) throw std::runtime_error("FFmpeg could not decode audio asset: " + path);
        SourceAudio audio = loadWav(temporary.string()); std::error_code error; std::filesystem::remove(temporary, error); return audio;
    }
    throw std::runtime_error("Unsupported audio asset format: " + extension);
}

} // namespace lyra
