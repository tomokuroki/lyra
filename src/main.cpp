#include "common.hpp"
#include "parser.hpp"
#include "synth.hpp"
#include "export.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace lyra;

static void printHelp(const char* prog) {
    std::cout <<
R"(Lyra - A minimal music programming language

Usage:
  )" << prog << R"( [options] <file.lyra> [output]

Options:
  -f, --format wav|midi     Output format (default: wav)
  -r, --rate <Hz>           Sample rate (default: 44100)
  -b, --bits 16|8           Bit depth for WAV (default: 16)
  -w, --wave <type>         Default waveform
  -h, --help                Show this help

Waveforms: sine, square, triangle, saw, pulse, noise
Instruments: piano, electric_piano, organ, music_box, glockenspiel,
             strings, brass, flute, guitar, electric_guitar, synth_bass
Drum kits: standard, rock, electronic, retro, orchestral

Examples:
  )" << prog << R"( song.lyra
  )" << prog << R"( -f midi song.lyra
  )" << prog << R"( -r 22050 -w square song.lyra out.wav
)";
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    Config cfg;
    std::string inputFile;
    std::string outputFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        }
        else if ((arg == "-f" || arg == "--format") && i + 1 < argc) {
            std::string f = toLower(argv[++i]);
            cfg.format = (f == "midi" || f == "mid") ? ExportFormat::MIDI : ExportFormat::WAV;
        }
        else if ((arg == "-r" || arg == "--rate") && i + 1 < argc) {
            cfg.sampleRate = std::stoi(argv[++i]);
        }
        else if ((arg == "-b" || arg == "--bits") && i + 1 < argc) {
            cfg.bits = std::stoi(argv[++i]);
        }
        else if ((arg == "-w" || arg == "--wave") && i + 1 < argc) {
            cfg.wave = parseWave(argv[++i]);
        }
        else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
        else if (inputFile.empty()) {
            inputFile = arg;
        }
        else {
            outputFile = arg;
        }
    }

    if (inputFile.empty()) {
        printHelp(argv[0]);
        return 1;
    }

    if (outputFile.empty()) {
        size_t pos = inputFile.find_last_of("/\\");
        std::string base = (pos == std::string::npos) ? inputFile : inputFile.substr(pos + 1);
        size_t dot = base.find_last_of('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        outputFile = base + (cfg.format == ExportFormat::MIDI ? ".mid" : ".wav");
    }

    try {
        std::ifstream in(inputFile);
        if (!in) throw std::runtime_error("Cannot open: " + inputFile);

        std::stringstream buffer;
        buffer << in.rdbuf();

        Parser parser;
        parser.config = cfg;
        parser.parse(buffer.str());

        std::cout << "Lyra | tempo=" << parser.config.tempo
                  << " | events=" << parser.events.size()
                  << " | format=" << (cfg.format == ExportFormat::MIDI ? "MIDI" : "WAV")
                  << std::endl;

        if (cfg.format == ExportFormat::MIDI) {
            writeMidi(outputFile, parser.events, parser.config);
            std::cout << "Created: " << outputFile << std::endl;
        } else {
            auto samples = generateSamples(parser.events, parser.config);
            writeWav(outputFile, samples, parser.config);
            double sec = samples.size() / static_cast<double>(parser.config.sampleRate);
            std::cout << "Created: " << outputFile << " (" << sec << "s)" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
