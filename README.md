# Lyra

**A minimal, extensible music programming language** written in pure C++17.

The active programmable-DAW roadmap is tracked in
[docs/TODO_DAW_RU.md](docs/TODO_DAW_RU.md).

## Lyra 3.0

Lyra 3 adds mutable indexed lists, collection operations, scalar functions
with `return`, nested calls, `while`, `break`, `continue`, safer diagnostics,
and a recursive `lyra test` project runner. See the
[Russian Lyra 3 guide](docs/LANGUAGE_3_RU.md).

Lyra lets you compose music with a simple text-based syntax and render it to
WAV, MIDI, AIFF, JSON, FLAC, MP3, OGG, or AAC/M4A.
Designed to be easy to read, easy to extend, and easy to contribute to.

## Lyra 2.1 language front-end

Lyra now supports variables and constants, numeric expressions, parameterized
patterns and functions, `repeat`, compile-time `if`, transposition, imports,
music-domain objects, instrument/effect inheritance, and an open standard
library. Existing Lyra 1 files remain compatible. See the
[Lyra 2 Russian guide](docs/LANGUAGE_2_RU.md) and
[complete showcase](examples/lyra2/showcase.lyra).

Version 2.1 adds lists, `for`/`range`, time and key signatures, scale degrees,
symbolic chords, per-event velocity, configurable envelopes, filtering, and drive.

```lyra
tempo 120
wave square

track melody {
  note C4 0.5
  note E4 0.5
  note G4 1
}

track drums {
  kick 0.5
  snare 0.5
}
```

## Features

- Multi-track composition (melody, bass, harmony, drums)
- Waveforms: `sine`, `square`, `triangle`, `saw`, `pulse`, `noise`
- Procedural instruments: piano, electric piano, organ, music box, strings,
  brass, flute, guitars, synth bass, and more
- 16 drum sounds and five kits: standard, rock, electronic, retro, orchestral
- Nine sound modes: 4-bit, 8-bit, 16-bit, 32-bit, 64-bit, tracker/Amiga,
  FM synthesis, modern chiptune, and clean modern
- Ordered DSP effects including parametric EQ, dynamics, modulation, reverb,
  delay, saturation, stereo width, and auto-pan
- Interpreter-style `run`, `check`, and `init` commands
- Release WAV export up to 192 kHz: 8/16/24-bit PCM or 32-bit IEEE float
- Track panning, fades, metadata, and streaming/CD/hi-res master presets
- Loops and chords
- Native export to **WAV**, **MIDI**, **AIFF**, and event **JSON**
- **FLAC**, **MP3**, **OGG**, and **AAC/M4A** export through FFmpeg
- Per-track stem export and offline peak/RMS/LUFS/spectrum/waveform analysis
- Zero external dependencies for the language and native exporters
- Clean modular architecture
- Validated Project IR with named tracks, mixer state, markers, and automation

## Quick Start

### Build

```bash
make
# or
mkdir build && cd build && cmake .. && make
```

### Run

```bash
./lyra examples/hello.lyra
./lyra -f midi examples/full_song.lyra
./lyra -f json examples/full_song.lyra
./lyra -f flac examples/full_song.lyra
./lyra -r 192000 -b 32 song.lyra out.wav
./lyra --stems stems --analysis meters.json song.lyra master.wav
```

## Documentation

| Language | Document |
|----------|----------|
| English  | [Language Reference](docs/LANGUAGE.md) |
| Русский  | [Справка по языку](docs/LANGUAGE_RU.md) |
| English  | [Architecture](docs/ARCHITECTURE.md) |

## Project Structure

```
lyra/
├── src/
│   ├── common.hpp      # Shared types & utilities
│   ├── project.hpp/cpp # Validated song intermediate representation
│   ├── dsp.hpp/cpp     # Ordered insert/bus/master effects
│   ├── audio_file.hpp/cpp # Audio asset decoding
│   ├── parser.hpp/cpp  # Command stream → Project IR
│   ├── synth.hpp/cpp   # Synthesis engine
│   ├── analysis.hpp/cpp # Offline audio meters and analyzer data
│   ├── export.hpp/cpp  # Audio, MIDI, and event-data back-end
│   └── main.cpp        # CLI
├── examples/           # Example compositions
├── docs/               # Full documentation
├── tests/              # Regression and integration tests
├── Makefile
├── CMakeLists.txt
├── LICENSE
└── CONTRIBUTING.md
```

This layout follows the spirit of well-structured language implementations:
clear separation between parsing, execution/synthesis, and output.

## Extending Lyra

See [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

| Want to add…       | Start here              |
|--------------------|-------------------------|
| New waveform       | `common.hpp` + `synth.cpp` |
| New language command | `parser.cpp`          |
| New drum sound     | `synth.cpp`             |
| New file format    | `export.cpp`            |

## License

Lyra is released under the license contained in the [LICENSE](LICENSE) file.
(see [LICENSE](LICENSE)).  

You are free to use, modify, and distribute Lyra, including in commercial
and closed-source projects, as long as you retain the copyright notice and
license text.

## Author

[tomokuroki](https://github.com/tomokuroki)
