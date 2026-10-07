# Lyra 3.0 — план развития до программируемой DAW

Версия остаётся **3.0.0** до отдельного решения автора. Этот файл — рабочий
чек-лист: `[x]` готово и покрыто проверкой, `[~]` работает частично, `[ ]` ещё
не реализовано. Пункт переводится в `[x]` только вместе с примером/тестом.

## 1. Музыкальный язык и timeline

- [x] Ноты: pitch/octave, длительность, velocity, transpose.
- [x] Расширенные chord symbols, положительные/отрицательные inversions и arpeggio.
- [~] Tempo map влияет на audio render и MIDI export; нужны плавные tempo curves.
- [x] Размеры 2/4/8/16 в знаменателе.
- [x] Grid: степени двойки 1/1…1/128 и triplets.
- [~] Swing/shuffle 50–75%; нужны именованные groove templates.
- [x] Повторяемые patterns и loops.
- [~] Pattern placements и `at <beat>` есть в Project IR; нужен clip-level playlist API.
- [x] Именованные sections с началом и длиной в Project IR.
- [x] Scale degrees: major/minor и 11 дополнительных scales/modes.
- [x] Scale lock и chord generator через расширенные symbols.
- [x] Arpeggiator, probability, seeded random и humanize/randomize.

## 2. Синтез

- [x] sine/square/saw/triangle/pulse/noise.
- [ ] Формульный custom oscillator.
- [x] Multi-oscillator, detune и unison 2/4/8/16.
- [~] Настоящая FM ratio/index готова; нужна многооператорная FM matrix.
- [~] AM готова; нужны wavetable и additive synthesis.
- [~] Subtractive chain: oscillator/filter/amp есть частично.
- [ ] Karplus–Strong и physical modeling.
- [x] White, pink, brown и blue noise generators.
- [x] Полный ADSR: attack/decay/sustain/release.
- [x] Pitch/filter envelopes и modulation matrix с sine/triangle/random LFO.

## 3. Sampler и audio clips

- [x] Native WAV/AIFF sampler, relative asset resolver; FLAC/MP3/OGG через FFmpeg.
- [x] One-shot, loop samples и sample layers через перекрывающиеся clips.
- [x] Sample slicing через trim и crossfade loops.
- [~] Reverse/resampling/pitch/time stretch готовы базово; нужен phase-vocoder quality mode.
- [x] Audio clips на timeline: trim, fades и clip gain.
- [ ] Запись, count-in и realtime monitoring.

## 4. Эффекты и mastering

- [x] LP/HP/BP/notch filters с resonance.
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
- [x] Sidechain compression и parallel processing через send buses.
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

## 8. Интерфейс

GUI не планируется: Lyra — язык программирования для создания музыки. Основной интерфейс — исходные `.lyra`-файлы, CLI и интеграции с редакторами кода.

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
