# Lyra 3.0 — план развития до программируемой DAW

Версия остаётся **3.0.0** до отдельного решения автора. Этот файл — рабочий
чек-лист: `[x]` готово и покрыто проверкой, `[~]` работает частично, `[ ]` ещё
не реализовано. Пункт переводится в `[x]` только вместе с примером/тестом.

## 1. Музыкальный язык и timeline

- [x] Ноты: pitch/octave, длительность, velocity, transpose.
- [~] Аккорды: символы и качества готовы; нужны inversions и arpeggio.
- [~] Tempo map влияет на audio render и MIDI export; нужны плавные tempo curves.
- [x] Размеры 2/4/8/16 в знаменателе.
- [x] Grid: степени двойки 1/1…1/128 и triplets.
- [~] Swing/shuffle 50–75%; нужны именованные groove templates.
- [x] Повторяемые patterns и loops.
- [~] Pattern placements и `at <beat>` есть в Project IR; нужен clip-level playlist API.
- [x] Именованные sections с началом и длиной в Project IR.
- [x] Scale degrees и major/minor.
- [ ] Остальные scales/modes, scale lock и chord generator.
- [ ] Arpeggiator, probability, seeded random и humanize.

## 2. Синтез

- [x] sine/square/saw/triangle/pulse/noise.
- [ ] Формульный custom oscillator.
- [ ] Multi-oscillator, detune и unison 2/4/8/16.
- [~] FM sound mode; нужна настоящая матрица FM.
- [ ] AM, wavetable и additive synthesis.
- [~] Subtractive chain: oscillator/filter/amp есть частично.
- [ ] Karplus–Strong и physical modeling.
- [~] White noise есть; нужны pink/brown/blue.
- [~] Envelope: attack/release есть; нужны decay/sustain.
- [ ] Pitch/filter envelopes, LFO и modulation matrix.

## 3. Sampler и audio clips

- [ ] WAV/AIFF/FLAC sampler и asset resolver.
- [ ] One-shot, loop samples и sample layers.
- [ ] Sample slicing и crossfade loops.
- [ ] Reverse, resampling, pitch shift и time stretch.
- [ ] Audio clips на timeline: trim, fades и clip gain.
- [ ] Запись, count-in и realtime monitoring.

## 4. Эффекты и mastering

- [~] LP/HP готовы; нужны BP/notch + resonance.
- [ ] Parametric EQ.
- [~] Reverb/delay готовы в базовом виде; нужны room/hall/plate и ping-pong.
- [~] Chorus, saturation и bitcrusher готовы; нужны flanger и phaser.
- [~] Compressor, limiter и gate готовы; нужны expander и de-esser.
- [~] Stereo widener и auto-pan готовы; нужна pitch correction.
- [ ] Oversampling для нелинейных эффектов.
- [ ] Mastering chain и loudness targets.
- [ ] Peak/LUFS/spectrum/oscilloscope/stereo meters.

## 5. Mixer и routing

- [x] Именованные mixer channels, volume/gain, pan, mute/solo.
- [x] Ordered insert FX chains на track/bus/master.
- [x] Именованные buses и post-insert sends.
- [ ] Sidechain и parallel processing.
- [x] Master channel как отдельный routing node.
- [~] Automation: volume/pan/cutoff/drive + linear/step.
- [ ] Pitch/tempo/любые FX parameters + ease/Bezier.

## 6. MIDI, форматы и экспорт

- [x] MIDI export.
- [ ] MIDI import, live input и MIDI CC.
- [x] WAV/AIFF/JSON export.
- [x] FLAC/MP3/OGG через FFmpeg.
- [ ] AAC export.
- [ ] Stem export.
- [~] 44.1/48/96 kHz и 8/16/24 bit; нужны 192 kHz и 32-bit float.
- [~] `.lyra` source project; нужны manifest, assets и portable bundle.
- [x] Presets, reusable modules и standard library.

## 7. Runtime, realtime и расширения

- [x] Functions, variables, constants, lists, conditions, repeat/for/while.
- [~] Диагностика с файлами и строками; нужен typed AST/source map.
- [ ] Undo/redo command model.
- [ ] Live reload и dependency graph.
- [ ] Realtime playback, play-from-cursor и loop playback.
- [ ] Audio cache и incremental render.
- [ ] Multithreaded render и SIMD kernels.
- [ ] WASAPI/ASIO/CoreAudio, low latency и buffer sizes.
- [ ] Lyra plugin SDK.
- [ ] VST3/CLAP hosting и plugin automation.

## 8. GUI (после стабильного engine API)

- [ ] Piano roll.
- [ ] Playlist/timeline.
- [ ] Mixer.
- [ ] Automation editor.
- [ ] Waveform/sample editor.

## Порядок реализации

1. Timeline IR: tempo map, grid, swing, sections, pattern placements.
2. DSP graph: instruments → ordered inserts → buses/sends → master.
3. Полный synth/modulation/filter/effect API.
4. Sampler, assets и audio clips.
5. MIDI/audio I/O и realtime engine.
6. Export, stems, metering и mastering.
7. Plugin hosting, performance и GUI.

Каждый этап обязан сохранять старые `.lyra`, проходить `ctest` и иметь новый
исполняемый пример в `examples/lyra3`.
