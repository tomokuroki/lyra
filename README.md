# Lyra

**A minimal, extensible music programming language** written in pure C++17.

Lyra lets you compose music with a simple text-based syntax and render it to WAV or MIDI.  
Designed to be easy to read, easy to extend, and easy to contribute to.

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
- Drum synthesis: `kick`, `snare`, `hihat`, `tom`
- Loops and chords
- Export to **WAV** and **MIDI**
- Zero external dependencies
- Clean modular architecture

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
./lyra -r 22050 -w square song.lyra out.wav
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
│   ├── parser.hpp/cpp  # Language front-end
│   ├── synth.hpp/cpp   # Synthesis engine
│   ├── export.hpp/cpp  # WAV & MIDI back-end
│   └── main.cpp        # CLI
├── examples/           # Example compositions
├── docs/               # Full documentation
├── tests/              # (placeholder for future tests)
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
