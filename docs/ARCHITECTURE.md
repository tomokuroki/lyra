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
│    Parser     │   produces a validated Project IR
└───────────────┘
        │ tracks + mixer + markers + automation
        ▼
┌───────────────┐
│  Project IR   │   produces a deterministic render event view
└───────────────┘
        │ flattened NoteEvent sequence
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

`Project` is the durable intermediate representation (IR). It owns named
tracks, mixer channels, markers and automation lanes. `NoteEvent` is the
small render-level IR: every event retains its track id, lowered-command line and
absolute start time in beats. The synthesizer and exporters intentionally
consume the flattened event view, keeping DSP independent of language syntax.

`Project::validate()` is the semantic boundary. Invalid ownership, timing,
automation order and mixer values are rejected before rendering.

## Module responsibilities

| File            | Role                                      | Extension point                      |
|-----------------|-------------------------------------------|--------------------------------------|
| `frontend.*`    | Lyra 2 objects, imports and expansion     | New high-level language constructs   |
| `common.hpp`    | Types, note→frequency, string helpers     | New enums, shared utilities          |
| `project.*`     | Song IR, validation and render lowering   | Tracks, arrangement, automation      |
| `dsp.*`         | Ordered effect processing primitives      | New effects and DSP algorithms       |
| `audio_file.*`  | WAV/AIFF and optional FFmpeg asset decode  | Samplers and audio clip formats      |
| `parser.*`      | Text → list of `NoteEvent`                | New syntax / commands                |
| `synth.*`       | `NoteEvent` → PCM samples                 | New synthesis, effects, stereo       |
| `export.*`      | PCM / events → audio and data files       | New codecs and event formats         |
| `main.cpp`      | Command-line interface                    | New flags, batch mode, REPL          |

## Design principles

1. **Zero-dependency core** – the language and native exporters use only C++17;
   optional compressed export delegates to FFmpeg.
2. **Single responsibility** – each translation unit does one job.
3. **Compatible lowering** – Lyra 2/3 constructs lower to the stable event language.
4. **Absolute-time model** – events carry their own start beat; the mixer stays simple.
5. **Fail fast** – invalid programs stop before audio export.
6. **Easy to build and hack** – `make` or CMake, no complex build system.

## Audio routing

Projects with a routing graph render each track into an independent floating-
point buffer. Track inserts run in source order. Post-insert sends feed named
buses; each bus has its own ordered insert chain and gain. Bus returns join the
dry tracks at the master, whose insert chain runs last. Only then is the master
peak target applied. This topology keeps routing explicit and deterministic.

## Future directions

- A typed syntax tree before Project lowering
- Tempo-map aware sample scheduling
- Effect graphs, buses and sends in the Project IR
- Lexical local scopes and return values for runtime functions
- Real-time playback
- Plugin system for custom waveforms / effects
- Test suite under `tests/`
