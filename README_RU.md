# Lyra

**Минималистичный и расширяемый язык программирования для музыки** на чистом C++17.

План развития до программируемой DAW: [docs/TODO_DAW_RU.md](docs/TODO_DAW_RU.md).

## Lyra 3.0

Третья версия добавляет изменяемые и индексируемые списки, `len`, `push`,
`remove`, `pop`, функции с `return`, вложенные числовые вызовы, `while`,
`break`, `continue`, защиту от бесконечных циклов и команду тестирования
проектов `lyra test`.

Документация: **[ядро Lyra 3.0](docs/LANGUAGE_3_RU.md)**.

## Lyra 2.1

Lyra теперь имеет отдельный фронтенд языка: переменные и константы, выражения,
параметризованные паттерны и функции, `repeat`, `if`, транспозицию, импорты,
объекты `Song`/`Track`/`Instrument`/`Pattern`/`Effect`, наследование музыкальных
пресетов и открытую стандартную библиотеку. Старый синтаксис совместим.

В 2.1 также добавлены списки, `for`/`range`, размеры такта, тональности,
ступени гаммы, аккорды по символам, MIDI-метаданные тональности/размера,
индивидуальная `velocity` и параметры синтеза `attack`, `release`, `cutoff`,
`drive`.

```lyra
import "std/all.lyra"
const BPM = 128

instrument MyLead extends BrightLead {
  volume 65
}

song Demo {
  tempo $BPM
  track lead uses MyLead {
    repeat 4 {
      play major_arp(C5, E5, G5, 0.5)
    }
  }
}
```

Подробно: **[Lyra 2.1 — программирование музыки](docs/LANGUAGE_2_RU.md)**.

Lyra позволяет писать музыку простым текстом и экспортировать её в WAV, MIDI,
AIFF, JSON, FLAC, MP3 или OGG.
Язык спроектирован так, чтобы его было легко читать, расширять и развивать.

## Возможности

- Несколько дорожек (мелодия, бас, гармония, ударные)
- Волны: `sine`, `square`, `triangle`, `saw`, `pulse`, `noise`
- Инструменты: пианино, электропианино, орган, музыкальная шкатулка,
  колокольчики, струнные, духовые, флейта, гитары и синт-бас
- 16 ударных: от `kick` и `snare` до `crash`, `ride`, `timpani` и `impact`
- Наборы ударных: `standard`, `rock`, `electronic`, `retro`, `orchestral`
- Девять режимов звучания: `4bit`, `8bit`, `16bit`, `32bit`, `64bit`,
  `tracker`, `fm`, `chiptune_modern` и чистый `modern`
- Встроенные `reverb` и ритмический `delay`
- Команды запуска в стиле интерпретатора: `run`, `check`, `init`
- Публикационный WAV: стерео, настоящий 24-bit PCM, 48/96 кГц
- Панорама дорожек, мастер-пик, fade-in/fade-out и WAV-метаданные
- Пресеты мастеринга: `streaming`, `cd`, `hires`
- Циклы и аккорды
- Нативный экспорт в **WAV**, **MIDI**, **AIFF** и событийный **JSON**
- Экспорт в **FLAC**, **MP3** и **OGG** через FFmpeg
- Язык и нативные форматы работают без внешних зависимостей
- Чистая модульная архитектура
- Проверяемая модель проекта: дорожки, mute/solo, gain, маркеры и автоматизация

## Быстрый старт

```bash
make
./lyra examples/hello.lyra
./lyra examples/retro_boss.lyra
./lyra examples/echoes_of_the_last_star.lyra
./lyra examples/sound_eras/01_4bit_toy_march.lyra
```

Как у Python, имя файла можно передать сразу:

```bash
lyra main.lyra
lyra run main.lyra
lyra check main.lyra
lyra init main.lyra
```

## Документация

- **[English – Language Reference](docs/LANGUAGE.md)** (основная)
- **[Русский – Справка по языку](docs/LANGUAGE_RU.md)**

## Структура проекта

См. [README.md](README.md).

## Лицензия

MIT.
