# Architecture

Lyra follows a clean pipeline inspired by the separation of concerns found in
mature language implementations (parser → intermediate representation → backend).

## Pipeline

```
.lyra source text
        │
        ▼
┌───────────────┐
│    Parser     │   produces  vector<NoteEvent>
└───────────────┘
        │
        ▼
┌───────────────┐
│    Synth      │   produces  vector<int16_t> (PCM)
└───────────────┘
        │
        ▼
┌───────────────┐
│    Export     │   writes    .wav  or  .mid
└───────────────┘
```

`NoteEvent` is the intermediate representation (IR).  
Every musical event knows its absolute start time in beats.  
The synthesizer mixes overlapping events from multiple tracks.

## Module responsibilities

| File            | Role                                      | Extension point                      |
|-----------------|-------------------------------------------|--------------------------------------|
| `common.hpp`    | Types, note→frequency, string helpers     | New enums, shared utilities          |
| `parser.*`      | Text → list of `NoteEvent`                | New syntax / commands                |
| `synth.*`       | `NoteEvent` → PCM samples                 | New synthesis, effects, stereo       |
| `export.*`      | PCM / events → files                      | New formats (FLAC, OGG, …)           |
| `main.cpp`      | Command-line interface                    | New flags, batch mode, REPL          |

## Design principles

1. **Zero external dependencies** – only the C++17 standard library.
2. **Single responsibility** – each translation unit does one job.
3. **Absolute-time model** – events carry their own start beat; the mixer is trivial.
4. **Fail fast** – errors always include the source line number.
5. **Easy to build and hack** – `make` or CMake, no complex build system.

## Future directions

- Proper AST instead of immediate event emission
- Variables, macros, and functions in the language
- Real-time playback
- Plugin system for custom waveforms / effects
- Test suite under `tests/`
