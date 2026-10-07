# Lyra regression checks

Build Lyra, then run the language and compatibility checks:

```text
lyra --version
lyra check examples/lyra2/showcase.lyra
lyra expand examples/lyra2/showcase.lyra
lyra check examples/lyra2/music_theory.lyra
lyra test examples/lyra3
lyra check examples/ambient/quiet_orbit.lyra
lyra check examples/boss/last_bloom_battle.lyra
lyra -f json examples/hello.lyra hello.json
lyra -f aiff examples/hello.lyra hello.aiff
```

The Lyra 2 showcase covers imports, `let`/`set`/`const`, expressions,
instrument and effect inheritance, parameterized patterns and functions,
`repeat`, `if/else`, transposition, `Song`, `Track`, and the standard library.
The ambient and boss checks protect Lyra 1 source compatibility.
The music-theory example covers lists, `for`/`range`, time/key signatures,
scale degrees, symbolic chords, velocity, envelopes, filtering, and drive.
The Lyra 3 suite covers mutable collection indexing, `len`, scalar `return`,
nested expressions, `while`, `break`, and `continue`.
`project_ir.lyra` additionally covers the Project IR boundary, named tracks,
track gain, mute, markers, and continuous/step automation.

With a CMake build the complete suite is also registered with CTest:

```text
ctest --test-dir build -C Release --output-on-failure
```
