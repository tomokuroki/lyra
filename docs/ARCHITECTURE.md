# Architecture

Lyra follows a clean pipeline inspired by the separation of concerns found in
mature language implementations (parser → intermediate representation → backend).

## Pipeline

```
.lyra source text + imported modules
        │
        ▼
┌───────────────┐
│ Lyra 2 Frontend│  variables, objects, patterns, expressions, transpose
└───────────────┘
        │ expanded command stream
        ▼
┌───────────────┐
│    Parser     │   produces  vector<NoteEvent>
└───────────────┘
        │
        ▼
┌───────────────┐
│    Synth      │   produces  floating-point stereo AudioBuffer
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
| `frontend.*`    | Lyra 2 objects, imports and expansion     | New high-level language constructs   |
| `common.hpp`    | Types, note→frequency, string helpers     | New enums, shared utilities          |
| `parser.*`      | Text → list of `NoteEvent`                | New syntax / commands                |
| `synth.*`       | `NoteEvent` → PCM samples                 | New synthesis, effects, stereo       |
| `export.*`      | PCM / events → audio and data files       | New codecs and event formats         |
| `main.cpp`      | Command-line interface                    | New flags, batch mode, REPL          |

## Design principles

1. **Zero-dependency core** – the language and native exporters use only C++17;
   optional compressed export delegates to FFmpeg.
2. **Single responsibility** – each translation unit does one job.
3. **Compatible lowering** – Lyra 2 constructs lower to the stable Lyra 1 event language.
4. **Absolute-time model** – events carry their own start beat; the mixer stays simple.
5. **Fail fast** – invalid programs stop before audio export.
6. **Easy to build and hack** – `make` or CMake, no complex build system.

## Future directions

- Source locations and a typed AST for richer diagnostics
- Lexical local scopes and return values for runtime functions
- Real-time playback
- Plugin system for custom waveforms / effects
- Test suite under `tests/`
