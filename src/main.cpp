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
  )" << prog << R"( <file.lyra> [output]        Run a song, like: python main.py
  )" << prog << R"( run <file.lyra> [output]    Explicit run command
  )" << prog << R"( check <file.lyra>            Validate without rendering
  )" << prog << R"( init [file.lyra]             Create a starter song

Options:
  -f, --format wav|midi     Output format (default: wav)
  -r, --rate <Hz>           Sample rate (default: 44100)
  -b, --bits 16|8           Bit depth for WAV (default: 16)
  -w, --wave <type>         Default waveform
  -s, --sound <mode>        Sound era/style (see list below)
  -v, --version             Show Lyra version
  -h, --help                Show this help

Waveforms: sine, square, triangle, saw, pulse, noise
Instruments: piano, electric_piano, organ, music_box, glockenspiel,
             strings, brass, flute, guitar, electric_guitar, synth_bass
Drum kits: standard, rock, electronic, retro, orchestral
Sound modes: 4bit, 8bit, 16bit, 32bit, 64bit, tracker, fm,
             chiptune_modern, modern

Examples:
  )" << prog << R"( main.lyra
  )" << prog << R"( run main.lyra music.wav
  )" << prog << R"( check main.lyra
  )" << prog << R"( init main.lyra
  )" << prog << R"( -f midi main.lyra
  )" << prog << R"( -r 22050 -w square song.lyra out.wav
)";
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    Config cfg;
    std::string inputFile;
    std::string outputFile;
    enum class Command { Run, Check };
    Command command = Command::Run;
    int firstArg = 1;

    if (argc > 1) {
        std::string first = toLower(argv[1]);
        if (first == "run" || first == "render") {
            firstArg = 2;
        } else if (first == "check") {
            command = Command::Check;
            firstArg = 2;
        } else if (first == "init") {
            std::string filename = argc > 2 ? argv[2] : "main.lyra";
            std::ifstream existing(filename);
            if (existing.good()) {
                std::cerr << "Error: file already exists: " << filename << std::endl;
                return 1;
            }
            std::ofstream starter(filename);
            if (!starter) {
                std::cerr << "Error: cannot create: " << filename << std::endl;
                return 1;
            }
            starter << R"(# My first Lyra song
tempo 120
sound 16bit
reverb 20

track piano {
  instrument piano
  chord C4 E4 G4 1
  chord F4 A4 C5 1
  chord G4 B4 D5 1
  chord C4 E4 G4 1
}

track drums {
  drumkit standard
  loop 2 {
    kick 0.5
    hihat 0.5
    snare 0.5
    hihat 0.5
  }
}
)";
            std::cout << "Created starter song: " << filename << std::endl;
            std::cout << "Run it with: " << argv[0] << " " << filename << std::endl;
            return 0;
        }
    }

    for (int i = firstArg; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        }
        else if (arg == "-v" || arg == "--version") {
            std::cout << "Lyra 1.1.0" << std::endl;
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
            if (cfg.bits != 8 && cfg.bits != 16) {
                std::cerr << "Bit depth must be 8 or 16\n";
                return 1;
            }
        }
        else if ((arg == "-w" || arg == "--wave") && i + 1 < argc) {
            cfg.wave = parseWave(argv[++i]);
        }
        else if ((arg == "-s" || arg == "--sound") && i + 1 < argc) {
            cfg.soundMode = parseSoundMode(argv[++i]);
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
                  << " | sound=" << soundModeToString(parser.config.soundMode)
                  << " | format=" << (cfg.format == ExportFormat::MIDI ? "MIDI" : "WAV")
                  << std::endl;

        if (command == Command::Check) {
            std::cout << "OK: " << inputFile << " is valid" << std::endl;
            return 0;
        }

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
