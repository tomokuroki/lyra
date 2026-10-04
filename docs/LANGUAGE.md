# Lyra Language Reference

## Overview

Lyra is a domain-specific language (DSL) for music composition.  
A `.lyra` file is a sequence of commands that describe notes, timing, tracks and effects.  
The interpreter turns this description into audio (WAV) or MIDI.

## Commands

### Global settings

| Command | Description | Example |
|---------|-------------|---------|
| `tempo <bpm>` | Set tempo in beats per minute | `tempo 120` |
| `volume <0-100>` | Global or track volume | `volume 80` |
| `wave <type>` | Default waveform | `wave square` |
| `instrument <name>` | Select a procedural instrument | `instrument piano` |
| `drumkit <name>` | Select a drum kit | `drumkit rock` |
| `sound <mode>` | Select 8bit, 16bit, or modern rendering | `sound 16bit` |
| `reverb <0-100>` | Add room reflections | `reverb 25` |
| `delay <beats> <0-100>` | Add tempo-synced echo | `delay 0.75 15` |
| `master <preset>` | Select streaming, CD, or hi-res master | `master streaming` |
| `peak <dB>` | Set master peak from -12 to 0 dB | `peak -1` |
| `fadein`, `fadeout` | Master fades measured in beats | `fadeout 8` |
| `title`, `artist`, `album` | Embed WAV INFO metadata | `title "My Song"` |

### Notes and rests

| Command | Description | Example |
|---------|-------------|---------|
| `note <pitch> <beats>` | Play a single note | `note C4 0.5` |
| `rest <beats>` | Silence | `rest 0.25` |
| `chord <notes...> <beats>` | Play several notes together | `chord C4 E4 G4 1` |

**Pitch format:** `C4`, `C#4`, `Db4`, `F#5`, `Bb3` … (octave 0–8)

**Duration** is expressed in beats. At `tempo 120`:
- `1.0` = 0.5 seconds
- `0.5` = 0.25 seconds

### Tracks

```lyra
track melody {
  wave pulse
  volume 80
  note C4 1
  note E4 1
}

track bass {
  wave triangle
  note C3 2
}
```

Everything inside a `track { ... }` block runs in parallel with other tracks.  
Tracks are automatically mixed by the synthesis engine.

### Instruments

Use `instrument piano`, `electric_piano`, `organ`, `music_box`,
`glockenspiel`, `strings`, `brass`, `flute`, `guitar`, `electric_guitar`, or
`synth_bass`. These sounds are synthesized by Lyra without sample libraries.
Wave names remain accepted by `instrument` for backwards compatibility.

### Drums

Available commands are `kick`, `snare`, `hihat`, `openhat`, `low_tom`, `tom`,
`high_tom`, `clap`, `rimshot`, `crash`, `ride`, `cowbell`, `shaker`,
`tambourine`, `timpani`, and `impact`. The generic form `drum <name> [beats]`
is also accepted. Duration is optional and defaults to `0.25`.

Use `drumkit standard|rock|electronic|retro|orchestral` to change the
character of the percussion on the current track.

### Sound modes and effects

Available modes are `4bit`, `8bit`, `16bit`, `32bit`, `64bit`, `tracker`,
`fm`, `chiptune_modern`, and `modern`. They cover toy-like early chips,
classic console reduction, warm 16-bit chorus, ADPCM-like early sample
consoles, spacious later-console sound, Amiga modules, phase-modulation
synthesis, hybrid modern chiptune, and clean full-resolution rendering.
`reverb` and `delay` add ambience to the master.

### Release-ready WAV

`master streaming` selects stereo 48 kHz/24-bit PCM with a -1 dB peak.
`master cd` selects stereo 44.1 kHz/16-bit, and `master hires` selects stereo
96 kHz/24-bit. Override individual values with `samplerate`, `bitdepth`,
`channels`, and `peak`. Inside a track, `pan -100..100` places its sound in
the stereo field. Lyra embeds `title`, `artist`, and `album` in a WAV INFO
chunk; distributor-specific artwork and release identifiers are supplied at upload.

### Interpreter-style commands

```bash
lyra main.lyra
lyra run main.lyra song.wav
lyra check main.lyra
lyra init main.lyra
lyra --version
```

### Loops

```lyra
loop 4 {
  note C4 0.5
  note E4 0.5
}
```

The body is repeated the given number of times.  
Nested loops are not currently supported.

## Waveforms

| Name | Character |
|------|-----------|
| `sine` | Pure tone |
| `square` | Classic 8-bit |
| `triangle` | Soft chiptune |
| `saw` | Bright / buzzy |
| `pulse` | 25% duty cycle (very "game-like") |
| `noise` | White noise |

## Export formats

```bash
./lyra song.lyra                 # → song.wav
./lyra -f midi song.lyra         # → song.mid
./lyra -f aiff song.lyra         # → song.aiff
./lyra -f json song.lyra         # → song.json (parsed musical events)
./lyra -f flac song.lyra         # → song.flac (requires FFmpeg)
./lyra -f mp3 song.lyra          # → song.mp3 (requires FFmpeg)
./lyra -f ogg song.lyra          # → song.ogg (requires FFmpeg)
./lyra -r 22050 -b 8 song.lyra   # low-fi 8-bit WAV
```

WAV, MIDI, AIFF, and JSON are native exporters. Compressed formats use an
`ffmpeg` executable available in `PATH`.

## Complete example

```lyra
tempo 110
volume 75

track melody {
  wave pulse
  note C5 0.5
  note E5 0.5
  note G5 1
  rest 0.25
  note A4 1
}

track bass {
  wave triangle
  volume 50
  note C3 2
  note G2 2
}

track drums {
  volume 85
  kick 0.5
  hihat 0.25
  hihat 0.25
  snare 0.5
  hihat 0.5
}
```

## Error handling

The parser reports the line number and a clear message:

```
Error: Line 12: usage: note <pitch> <beats>
```
