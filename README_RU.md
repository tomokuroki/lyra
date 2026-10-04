# Lyra

**Минималистичный и расширяемый язык программирования для музыки** на чистом C++17.

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
