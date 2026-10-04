# Lyra regression checks

Build Lyra, then run the language and compatibility checks:

```text
lyra --version
lyra check examples/lyra2/showcase.lyra
lyra expand examples/lyra2/showcase.lyra
lyra check examples/ambient/quiet_orbit.lyra
lyra check examples/boss/last_bloom_battle.lyra
```

The Lyra 2 showcase covers imports, `let`/`set`/`const`, expressions,
instrument and effect inheritance, parameterized patterns and functions,
`repeat`, `if/else`, transposition, `Song`, `Track`, and the standard library.
The ambient and boss checks protect Lyra 1 source compatibility.
