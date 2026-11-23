#include "../../../Include/platform/win32/console_utils_win32.h"
#include "../../../include/utils/console_utils.h"
#include <cstdio>

bool console_utils::InitializeConsole(int cols, int rows) {
    if (cols <= 0 || rows <= 0) {
        return false;
    }

    // Create a new console screen buffer
    HANDLE hNewBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    if (hNewBuffer == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Try smaller fonts to fit more rows (target: 150+ rows)
    // Try 4x8 first, then 5x8, then 6x12
    CONSOLE_FONT_INFOEX font;
    font.cbSize = sizeof(font);
    font.nFont = 0;
    font.FontFamily = FF_DONTCARE;
    font.FontWeight = FW_NORMAL;

    bool fontSet = false;
    const struct { short w, h; const wchar_t* name; } fonts[] = {
        {4, 8, L"Terminal"},      // Raster font, smallest
        {5, 8, L"Consolas"},      // Small Consolas
        {6, 10, L"Consolas"},     // Medium Consolas
        {6, 12, L"Consolas"},     // Original
    };

    for (size_t i = 0; i < sizeof(fonts)/sizeof(fonts[0]); ++i) {
        font.dwFontSize.X = fonts[i].w;
        font.dwFontSize.Y = fonts[i].h;
        wcscpy_s(font.FaceName, fonts[i].name);
        if (SetCurrentConsoleFontEx(hNewBuffer, FALSE, &font)) {
            fontSet = true;
            break;
        }
    }

    if (!fontSet) {
        // Fallback
        font.dwFontSize.X = 6;
        font.dwFontSize.Y = 12;
        wcscpy_s(font.FaceName, L"Consolas");
        SetCurrentConsoleFontEx(hNewBuffer, FALSE, &font);
    }

    // Set buffer size to requested (or max available)
    COORD bufferSize = { static_cast<SHORT>(cols), static_cast<SHORT>(rows) };
    if (!SetConsoleScreenBufferSize(hNewBuffer, bufferSize)) {
        CloseHandle(hNewBuffer);
        return false;
    }

    // Check max window with current font
    COORD maxWindow = GetLargestConsoleWindowSize(hNewBuffer);
    int winCols = cols;
    int winRows = rows;
    if (winCols > maxWindow.X) winCols = maxWindow.X;
    if (winRows > maxWindow.Y) winRows = maxWindow.Y;

    // Set window size
    SMALL_RECT windowRect = { 0, 0, static_cast<SHORT>(winCols - 1), static_cast<SHORT>(winRows - 1) };
    if (!SetConsoleWindowInfo(hNewBuffer, TRUE, &windowRect)) {
        CloseHandle(hNewBuffer);
        return false;
    }

    // Set as active screen buffer
    if (!SetConsoleActiveScreenBuffer(hNewBuffer)) {
        CloseHandle(hNewBuffer);
        return false;
    }

    // Also set as standard output handle
    SetStdHandle(STD_OUTPUT_HANDLE, hNewBuffer);

    // Maximize window to full screen
    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        // Remove window decorations for true fullscreen
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_BORDER);
        SetWindowLong(hwnd, GWL_STYLE, style);

        // Get screen size
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        // Set window to full screen
        SetWindowPos(hwnd, HWND_TOP, 0, 0, screenW, screenH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        ShowWindow(hwnd, SW_MAXIMIZE);

        // Force focus
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }

    // Hide cursor on new buffer
    CONSOLE_CURSOR_INFO cursorInfo = { 1, FALSE };
    SetConsoleCursorInfo(hNewBuffer, &cursorInfo);

    // Also hide cursor on original stdout buffer (in case it's different)
    HANDLE hOrigOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOrigOut != hNewBuffer && hOrigOut != INVALID_HANDLE_VALUE) {
        SetConsoleCursorInfo(hOrigOut, &cursorInfo);
    }

    // Force focus more aggressively (reuse hwnd from above)
    if (hwnd) {
        // Bring to front and activate
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        SetActiveWindow(hwnd);
        // Also try AttachThreadInput trick for focus
        DWORD foreThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        DWORD currThread = GetCurrentThreadId();
        if (foreThread != currThread) {
            AttachThreadInput(currThread, foreThread, TRUE);
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
            AttachThreadInput(currThread, foreThread, FALSE);
        }
    }

    return true;
}