# Lyra 2.0 — программирование музыки

Lyra 2 добавляет полноценный фронтенд языка поверх совместимого аудиодвижка
Lyra 1. Старые файлы продолжают работать, а новые конструкции разворачиваются
в ноты и события до синтеза.

## Переменные, константы и выражения

```lyra
let repeats = 3
set repeats = repeats + 1
const BPM = 128
const STEP = 0.5

tempo $BPM
note C5 ${STEP * 2}
```

- `let` создаёт изменяемую переменную.
- `set` меняет её значение.
- `const` запрещает последующее изменение.
- `$name` подставляет значение.
- `${expression}` вычисляет выражение.
- Доступны `+ - * / %`, сравнения, `&&`, `||`, `!` и скобки.

## Пользовательские инструменты и наследование

```lyra
instrument SoftLead {
  wave pulse
  volume 48
  pan 15
}

instrument BossLead extends SoftLead {
  volume 70
}

track lead uses BossLead {
  note D5 1
}
```

Объект-наследник получает команды родителя, а затем переопределяет нужные
свойства. Это предметная объектная модель Lyra: `Song`, `Track`, `Instrument`,
`Pattern` и `Effect` вместо универсальных классов, не связанных с музыкой.

## Эффекты как объекты

```lyra
effect Space {
  reverb 40
  delay 1.5 12
}

effect PublishedSpace extends Space {
  master streaming
  peak -1
}

use effect PublishedSpace
```

## Параметризованные паттерны и функции

```lyra
pattern motif(root, third, fifth, step) {
  note $root $step
  note $third $step
  note $fifth ${step * 2}
}

function ending(root, third, fifth, beats) {
  chord $root $third $fifth $beats
}

play motif(D5, F5, A5, 0.5)
call ending(D4, F4, A4, 4)
```

Паттерны и функции выполняются фронтендом во время сборки композиции. Они не
добавляют накладных расходов в WAV и MIDI.

## repeat, if и transpose

```lyra
repeat 4 {
  play motif(C5, E5, G5, 0.5)
}

if BPM >= 120 {
  volume 72
} else {
  volume 64
}

transpose 12 {
  play motif(C5, E5, G5, 0.5)
}
```

`if` вычисляется до рендера. `transpose` изменяет ноты и аккорды внутри блока,
сохраняя ритм и структуру паттерна.

## Song и Track

```lyra
song BattleTheme {
  tempo 140

  track lead uses BossLead {
    repeat 4 {
      play motif(D5, F5, A5, 0.5)
    }
  }
}
```

`Song` является корневым объектом композиции. `Track` задаёт параллельную
временную линию и получает свойства объекта `Instrument` через `uses`.

## Модули и import

```lyra
import "parts/drums.lyra"
import "std/all.lyra"
```

Обычный импорт ищется относительно текущего файла. Путь `std/` загружает
стандартную библиотеку. Фронтенд предотвращает повторную загрузку и сообщает
о циклических импортах. Для отдельно установленной библиотеки можно задать
переменную окружения `LYRA_STDLIB`.

## Стандартная библиотека

`import "std/all.lyra"` предоставляет:

- инструменты `SoftPiano`, `BrightLead`, `DeepBass`, `AmbientPad`, `WideStrings`;
- эффекты `SmallRoom`, `DreamSpace`, `StreamingRelease`;
- паттерны `major_arp`, `minor_arp`, `four_on_floor`, `rock_bar`;
- функцию `final_chord`.

Исходники библиотеки находятся в `stdlib/`, поэтому пользователь может читать,
копировать и расширять их без скрытой магии.

## Полный пример

См. `examples/lyra2/showcase.lyra`. Проверка и запуск:

```text
lyra check examples/lyra2/showcase.lyra
lyra expand examples/lyra2/showcase.lyra
lyra examples/lyra2/showcase.lyra
```

`expand` показывает низкоуровневый результат после импортов, выражений,
функций и паттернов. Это удобно для обучения и отладки программ Lyra.

