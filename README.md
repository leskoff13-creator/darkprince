# Тёмный принц — VST3 для FL Studio

Голос → тёмный, низкий, «королевский» тембр. Цепочка:
питч-шифтер вниз → саб-слой (октавой ниже) → сатурация → тёмный EQ (по спектру референса) → тёмный реверб → dry/wet → лимитер.

Ручки: ПИТЧ, САБ, МРАК, ГРЯЗЬ, ЭХО, МИКС, ВЫХОД. Три пресета (меню плагина в FL).

## Сборка без установки чего-либо (через GitHub)
1. Создай репозиторий на github.com, залей туда содержимое этой папки.
2. Вкладка **Actions** → workflow **Build VST3** → дождись зелёной галочки (~5–8 мин).
3. Скачай артефакт **DarkPrince-VST3** (внутри папка `Dark Prince.vst3`).

## Сборка локально (Windows)
Нужны Visual Studio 2022 (Desktop development with C++) и CMake.
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Результат: `build/DarkPrince_artefacts/Release/VST3/Dark Prince.vst3`
(и Standalone-версия рядом, чтобы быстро потестить).

## Установка
Скопируй папку `Dark Prince.vst3` в `C:\Program Files\Common Files\VST3\`
В FL Studio: Options → Manage plugins → Find installed plugins. Плагин появится в Installed → Effects → Dark Prince. Ставь на канал микшера.
Плагин сообщает задержку (~26 мс), FL компенсирует её сам.
