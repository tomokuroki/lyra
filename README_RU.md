# Lyra

**Минималистичный и расширяемый язык программирования для музыки** на чистом C++17.

## Lyra 2.0

Lyra теперь имеет отдельный фронтенд языка: переменные и константы, выражения,
параметризованные паттерны и функции, `repeat`, `if`, транспозицию, импорты,
объекты `Song`/`Track`/`Instrument`/`Pattern`/`Effect`, наследование музыкальных
пресетов и открытую стандартную библиотеку. Старый синтаксис совместим.

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

Подробно: **[Lyra 2.0 — программирование музыки](docs/LANGUAGE_2_RU.md)**.

Lyra позволяет писать музыку простым текстом и экспортировать её в WAV или MIDI.  
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
- Экспорт в **WAV** и **MIDI**
- Без внешних зависимостей
- Чистая модульная архитектура

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
