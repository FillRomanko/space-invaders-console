#ifndef SPACE_INVADERS_CONSOLE_CONSOLE_UTILS_WIN32_H
#define SPACE_INVADERS_CONSOLE_CONSOLE_UTILS_WIN32_H

#pragma once
#include <windows.h>

namespace win32_console {
    // Режимы шрифта
    enum FontMode {
        FONT_MODE_MENU = 0,  // Крупный шрифт для меню
        FONT_MODE_GAME = 1   // Мелкий шрифт для игры
    };

    // Смена шрифта консоли на лету для указанного буфера С ресайзом окна.
    // Возвращает true при успехе. Обновляет *outCols и *outRows новым размером окна в символах.
    bool SetConsoleFontMode(HANDLE hBuffer, FontMode mode, int* outCols, int* outRows);

    // Только смена шрифта БЕЗ ресайза окна/буфера (для вторичного буфера двойной буферизации).
    bool SetConsoleFontOnly(HANDLE hBuffer, FontMode mode);
}

#endif //SPACE_INVADERS_CONSOLE_CONSOLE_UTILS_WIN32_H