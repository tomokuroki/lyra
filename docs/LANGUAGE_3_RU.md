# Lyra 3.0 — ядро языка

Lyra 3 расширяет музыкальный DSL конструкциями обычного языка программирования,
сохраняя совместимость с Lyra 1 и 2.

## Изменяемые списки

```lyra
let notes = [C4, E4, G4]
push notes, B4
set notes[0] = D4
remove notes, 1
pop notes

note $notes[0] 1
note $notes[-1] 1
```

`len(notes)` возвращает длину списка. Отрицательные индексы считаются с конца.
Список, объявленный через `const`, изменять нельзя.

## Функции с return

```lyra
function twice(value) {
  return value * 2
}

function velocity(base, accent) {
  return base + accent
}

note C5 ${twice(0.5)} velocity ${velocity(60, 15)}
```

Возвращаемые функции работают в числовых выражениях, поддерживают вложенные
вызовы и ограничены глубиной 32 для защиты от бесконечной рекурсии. Музыкальные
функции без `return` продолжают работать как разворачиваемые блоки через `call`.

## while, break и continue

```lyra
let index = 0

while index < 8 {
  set index = index + 1

  if index == 2 {
    continue
  }
  if index > 5 {
    break
  }

  degree $index 4 0.5
}
```

`break` и `continue` также работают внутри `for` и нового `repeat`. Один `while`
ограничен 10 000 итерациями, чтобы музыкальная сборка не могла зависнуть.

## Проверка проекта

```text
lyra lint main.lyra
lyra test examples/lyra3
```

`lint` проверяет один файл. `test` принимает файл или каталог, рекурсивно находит
все `.lyra`, компилирует каждый и печатает `PASS`/`FAIL` и количество событий.

## Проект, микшер и автоматизация

Внутри Lyra 3 теперь есть полноценная модель проекта: именованные дорожки не
теряются после разбора кода, а события сохраняют дорожку и строку пониженного
потока команд.
Это фундамент для playlist, buses, sends, live reload и поканального рендера.

```lyra
track lead {
  gain -3
  pan 15
  mute false
  solo false

  automate volume at 0 35 linear
  automate volume at 4 100 linear
  automate cutoff at 0 900 step
  automate cutoff at 4 6000 linear

  marker intro 0
  note C5 1
  note E5 1
  note G5 2
}
```

- `gain <dB>` задаёт усиление дорожки от −96 до +24 dB;
- `mute` и `solo` принимают `true/false` или `on/off`;
- `automate` поддерживает `volume`, `pan`, `cutoff`, `drive` и кривые
  `linear`/`step`; точки записываются по возрастанию тактов;
- `marker <name> [beat]` добавляет маркер аранжировки.

Автоматизация применяется детерминированно при построении render-view, поэтому
один и тот же проект всегда создаёт одинаковый результат.

## Timeline: tempo map, grid, swing и sections

```lyra
tempo 120
tempo 96 at 16
grid 1/16
swing 60
section intro at 0 length 16
section verse at 16 length 32

track drums {
  at 16
  play verse_drums()
}
```

`tempo ... at ...` меняет реальную длительность и позицию событий при аудио-
рендере. `grid` принимает степени двойки от `1/1` до `1/128` и необязательное
слово `triplet`. `swing 50` означает прямой ритм, значения до `75` задерживают
каждую вторую ячейку. Sections сохраняются в Project IR и одновременно создают
маркеры начала частей.
`at <beat>` перемещает курсор дорожки, а каждый `play pattern(...)` сохраняется
как отдельное размещение в `arrangement`, не теряя при этом совместимый поток
нот для существующего синтезатора.

## DSP graph: inserts, buses, sends и master

```lyra
bus space {
  gain -6
  fx chorus time=16 feedback=0.2 wet=25
  fx reverb time=79 feedback=0.55 wet=45
}

master {
  fx compressor threshold=-14 ratio=3 wet=100
  fx limiter threshold=-1 wet=100
}

track lead {
  fx highpass cutoff=90 wet=100
  fx saturation drive=2.5 wet=65
  send space 28
  note C4 1
}
```

Порядок строк `fx` является порядком обработки. Send снимается после insert-
цепочки дорожки, обрабатывается цепочкой bus и смешивается с master. Поддержаны
`lowpass`, `highpass`, `distortion`, `saturation`, `bitcrusher`, `chorus`,
`delay`, `reverb`, `compressor`, `limiter`, `gate`, `stereo_width` и
`auto_pan`. Общий параметр `wet` задаётся в процентах; остальные параметры
передаются как `name=value`.

Исполняемый пример: `examples/lyra3/dsp_graph.lyra`.

Sidechain задаётся на дорожке-приёмнике и ссылается на именованную дорожку-
источник:

```lyra
track bass {
  sidechain drums amount=100 threshold=-30 ratio=8 attack=2 release=180
  note C2 4
}
```

Envelope follower использует отдельные attack/release и действительно изменяет
PCM; интеграционный тест сравнивает WAV с маршрутом и без него.

## Multi-oscillator synth, unison, FM/AM и ADSR

```lyra
track pad {
  osc saw level=65 detune=-4
  osc square level=25 detune=5 semitones=-12
  osc sine level=20 semitones=12
  unison 8 detune=14
  adsr 0.03 0.25 62 0.35
  fm 2 0.7
  am 3.5 18
  chord C4 E4 G4 B4 2
}
```

`osc` добавляет генераторы в порядке объявления. `level` задаётся в процентах,
`detune` — в центах, `semitones` — в полутонах. `unison` принимает
1/2/4/8/16 голосов. `fm` принимает ratio и modulation index, `am` — частоту
и глубину в процентах. `adsr` использует секунды для A/D/R и проценты для S.

Исполняемый пример: `examples/lyra3/synth_engine.lyra`.

## Диагностика

Ошибки высокоуровневого фронтенда теперь содержат путь исходного или
импортированного файла. Ошибки `break`/`continue` вне цикла, выхода за границу
списка, изменения `const`, деления на ноль и чрезмерной рекурсии останавливают
сборку до создания аудиофайла.

## Пример

Полная исполняемая демонстрация находится в
`examples/lyra3/language_core.lyra`.

