**Русский** | <a href="#Eng">[English]</a>
<div id="Rus"></div>

# Space Invaders (Console Edition)
Консольная реализация классической аркадной игры Space Invaders с элементами современных механик (динамическая сложность, система сохранений).

## Цель проекта
Создать производительную игру в консоли, используя исключительно **Windows API (Win32)**, без сторонних библиотек и STL-контейнеров, с жестким лимитом памяти и асинхронным управлением.

## Технологический стек и ограничения
*   **Язык**: C++
*   **Платформа**: Windows (x64)
*   **API**: Win32 (Windows API). **Запрещено**: Любые сторонние библиотеки, STL-контейнеры (`std::vector`, `std::string`, `std::map` и др.).
*   **Управление памятью**: **Критическое ограничение — 2 МБ** для всех переменных игры.
*   **Ввод/Вывод**: Асинхронный ввод через `ReadConsoleInput` / `PeekConsoleInput` (без блокировки основного потока). Рендеринг с двойной буферизацией (`CreateConsoleScreenBuffer`) для стабильных 24+ FPS.
*   **Звук**: Воспроизведение через `PlaySound` (WinMM) для .wav файлов.
*   **Сохранение**: Файловый ввод/вывод через `CreateFile` / `WriteFile` (Win32 API) для автосохранения и таблицы рекордов.

## Игровые механики
*   **Два режима**: «Свободный полет» (здоровье корабля) и «Защита» (разные здоровье корабля и базы).
*   **Динамическая сложность**: Скорость появления врагов адаптируется к успехам игрока и постепенно растет.
*   **Система очков**: За время выживания и уничтожение врагов.
*   **Пауза и автосохранение**: Автоматическое сохранение состояния раз в секунду.
*   **Таблица рекордов**: Сохраняется между сессиями.

## Сборка и запуск
1.  **Требования**: Компилятор MSVC (Visual Studio) или MinGW-w64, CMake 3.15+.
2.  **Сборка**:
    ```bash
    mkdir build && cd build
    cmake .. -G "Visual Studio 17 2022" # или "MinGW Makefiles"
    cmake --build . --config Release
    ```
    
<a href="#Rus">[Русский]</a> | **English**
<div id="Eng"></div>

# Space Invaders (Console Edition)
A console implementation of the classic arcade game Space Invaders with elements of modern mechanics (dynamic difficulty, save system).

## Project Goal
To create a high-performance console game using exclusively the Windows API (Win32), without third-party libraries or STL containers, with a strict memory limit and asynchronous control.

## Tech Stack and Limitations
* **Language**: C++
* **Platform**: Windows (x64)
* **API**: Win32 (Windows API). **Prohibited**: Any third-party libraries, STL containers (`std::vector`, `std::string`, `std::map`, etc.).
* **Memory Management**: **Critical limit - 2 MB** for all game variables.
* **Input/Output**: Asynchronous input via `ReadConsoleInput` / `PeekConsoleInput` (without blocking the main thread). Double-buffered rendering (`CreateConsoleScreenBuffer`) for a stable 24+ FPS.
* **Sound**: Playback via `PlaySound` (WinMM) for .wav files.
* **Saving**: File I/O via `CreateFile` / `WriteFile` (Win32 API) for autosaving and highscores.

## Game Mechanics
* **Two modes**: "Free Flight" (ship health) and "Defense" (different ship and base health).
* **Dynamic Difficulty**: Enemy spawn rate adapts to player performance and gradually increases.
* **Scoring System**: For surviving and eliminating enemies.
* **Pause and Autosave**: Automatically saves the state once per second.
* **Highscore**: Saved between sessions.

## Build and Run
1. **Requirements**: MSVC (Visual Studio) or MinGW-w64 compiler, CMake 3.15+.
2. **Build**:
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" # or "MinGW Makefiles"
cmake --build . --config Release
