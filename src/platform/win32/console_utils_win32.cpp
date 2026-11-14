#include "../../../Include/platform/win32/console_utils_win32.h"
#include "../../../include/utils/console_utils.h"

static bool FindMaxFontSize(HANDLE hConsole, int cols, int rows,
                            CONSOLE_FONT_INFOEX* outFont) {
    CONSOLE_FONT_INFOEX original;
    original.cbSize = sizeof(original);
    if (!GetCurrentConsoleFontEx(hConsole, FALSE, &original)) {
        return false;
    }

    CONSOLE_FONT_INFOEX best = original;
    bool found = false;

    int lo = 5;
    int hi = 72;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;

        CONSOLE_FONT_INFOEX test = original;
        test.dwFontSize.Y = static_cast<SHORT>(mid);
        test.dwFontSize.X = 0;

        if (!SetCurrentConsoleFontEx(hConsole, FALSE, &test)) {
            hi = mid - 1;
            continue;
        }

        COORD largest = GetLargestConsoleWindowSize(hConsole);
        if (largest.X >= cols && largest.Y >= rows) {
            best = test;
            found = true;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    if (!found) {
        SetCurrentConsoleFontEx(hConsole, FALSE, &original);
        return false;
    }

    SetCurrentConsoleFontEx(hConsole, FALSE, &best);
    *outFont = best;
    return true;
}

bool console_utils::InitializeConsole(int cols, int rows) {
    if (cols <= 0 || rows <= 0) {
        return false;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return false;
    }

    CONSOLE_FONT_INFOEX chosenFont;
    if (!FindMaxFontSize(hConsole, cols, rows, &chosenFont)) {
        return false;
    }

    if (!SetCurrentConsoleFontEx(hConsole, FALSE, &chosenFont)) {
        return false;
    }

    // Уменьшаем окно, чтобы буфер можно было свободно менять.
    SMALL_RECT minWin = { 0, 0, 0, 0 };
    if (!SetConsoleWindowInfo(hConsole, TRUE, &minWin)) {
        return false;
    }

    COORD largest = GetLargestConsoleWindowSize(hConsole);
    if (!SetConsoleScreenBufferSize(hConsole, largest)) {
        return false;
    }

    SMALL_RECT maxWin = {
        0, 0,
        static_cast<SHORT>(largest.X - 1),
        static_cast<SHORT>(largest.Y - 1)
    };
    if (!SetConsoleWindowInfo(hConsole, TRUE, &maxWin)) {
        return false;
    }

    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER);
        SetWindowLong(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        ShowWindow(hwnd, SW_MAXIMIZE);
        UpdateWindow(hwnd);
    }

    // ВАЖНО: берём ФАКТИЧЕСКИЕ размеры видимой области консоли
    // ПОСЛЕ максимизации. Опираться на старый largest нельзя —
    // он был вычислен до SW_MAXIMIZE и уже не соответствует окну.
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    if (!GetConsoleScreenBufferInfo(hConsole, &buffer_info)) {
        return false;
    }

    auto winCols = static_cast<SHORT>(buffer_info.srWindow.Right - buffer_info.srWindow.Left + 1);
    auto winRows = static_cast<SHORT>(buffer_info.srWindow.Bottom - buffer_info.srWindow.Top + 1);

    // Если после maximize окно оказалось больше буфера —
    // аккуратно увеличиваем буфер и заново максимизируем.
    if (buffer_info.dwSize.X < winCols || buffer_info.dwSize.Y < winRows) {
        SMALL_RECT tmp = { 0, 0, 0, 0 };
        SetConsoleWindowInfo(hConsole, TRUE, &tmp);

        COORD newBuf = {
            (buffer_info.dwSize.X > winCols) ? buffer_info.dwSize.X : winCols,
            (buffer_info.dwSize.Y > winRows) ? buffer_info.dwSize.Y : winRows
        };
        if (!SetConsoleScreenBufferSize(hConsole, newBuf)) {
            return false;
        }

        if (hwnd) {
            ShowWindow(hwnd, SW_MAXIMIZE);
            UpdateWindow(hwnd);
        }

        if (!GetConsoleScreenBufferInfo(hConsole, &buffer_info)) {
            return false;
        }
        winCols = static_cast<SHORT>(buffer_info.srWindow.Right - buffer_info.srWindow.Left + 1);
        winRows = static_cast<SHORT>(buffer_info.srWindow.Bottom - buffer_info.srWindow.Top + 1);
    }

    // Центрируем рамку в реально видимой области окна.
    int offsetX = (winCols - cols) / 2;
    int offsetY = (winRows - rows) / 2;
    if (offsetX < 0) offsetX = 0;
    if (offsetY < 0) offsetY = 0;

    // srWindow может начинаться не с (0,0), если буфер больше окна.
    SHORT baseX = buffer_info.srWindow.Left;
    SHORT baseY = buffer_info.srWindow.Top;

    DWORD written = 0;
    // --- ВЕРХНЯЯ ЛИНИЯ ---
    for (int x = 0; x < cols; ++x) {
        COORD posT = {
            static_cast<SHORT>(baseX + offsetX + x),
            static_cast<SHORT>(baseY + offsetY)
        };
        FillConsoleOutputCharacterA(hConsole, '-', 1, posT, &written);
    }

    // Верхние углы
    COORD cornerTL = {
        static_cast<SHORT>(baseX + offsetX),
        static_cast<SHORT>(baseY + offsetY)
    };
    FillConsoleOutputCharacterA(hConsole, '+', 1, cornerTL, &written);

    COORD cornerTR = {
        static_cast<SHORT>(baseX + offsetX + cols - 1),
        static_cast<SHORT>(baseY + offsetY)
    };
    FillConsoleOutputCharacterA(hConsole, '+', 1, cornerTR, &written);

    for (int y = 1; y < rows; ++y) {
        COORD posL = {
            static_cast<SHORT>(baseX + offsetX),
            static_cast<SHORT>(baseY + offsetY + y)
        };
        FillConsoleOutputCharacterA(hConsole, '|', 1, posL, &written);

        COORD posR = {
            static_cast<SHORT>(baseX + offsetX + cols - 1),
            static_cast<SHORT>(baseY + offsetY + y)
        };
        FillConsoleOutputCharacterA(hConsole, '|', 1, posR, &written);
    }
    for (int x = 0; x < cols; ++x) {
        COORD posB = {
            static_cast<SHORT>(baseX + offsetX + x),
            static_cast<SHORT>(baseY + offsetY + rows - 1)
        };
        FillConsoleOutputCharacterA(hConsole, '-', 1, posB, &written);
    }

    COORD cornerBL = {
        static_cast<SHORT>(baseX + offsetX),
        static_cast<SHORT>(baseY + offsetY + rows - 1)
    };
    FillConsoleOutputCharacterA(hConsole, '+', 1, cornerBL, &written);

    COORD cornerBR = {
        static_cast<SHORT>(baseX + offsetX + cols - 1),
        static_cast<SHORT>(baseY + offsetY + rows - 1)
    };
    FillConsoleOutputCharacterA(hConsole, '+', 1, cornerBR, &written);

    return true;
}