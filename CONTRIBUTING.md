# Contributing to Lyra

Thank you for your interest in improving Lyra!

## Development setup

```bash
git clone https://github.com/tomokuroki/lyra.git
cd lyra
make
```

## Project layout (inspired by clean modular design)

```
src/
  common.hpp     Shared types, note parsing, utilities
  parser.*       Source text → NoteEvent list
  synth.*        NoteEvent list → PCM samples
  export.*       PCM / events → WAV or MIDI
  main.cpp       Command-line interface
```

## How to add a new feature

1. **New waveform**  
   - Add enum value in `common.hpp`  
   - Implement in `synth.cpp` → `sampleWave()`  
   - Accept the name in `parseWave()`

2. **New command**  
   - Add parsing branch in `parser.cpp`  
   - Create corresponding `NoteEvent`s

3. **New export format**  
   - Add function in `export.cpp` / `export.hpp`  
   - Wire it in `main.cpp`

4. **New drum sound**  
   - Add `DrumType` value  
   - Implement in `renderDrum()` inside `synth.cpp`

## Code style

- C++17
- Prefer clarity over cleverness
- Keep functions small and focused
- No external dependencies

## Pull requests

- One logical change per PR
- Update documentation if you change the language syntax
- Make sure `make` still succeeds

## License

By contributing you agree that your contributions are licensed under the same terms as the rest of the project (see LICENSE).
MIT License.
