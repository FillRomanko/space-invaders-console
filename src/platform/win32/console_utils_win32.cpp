#include "../../../Include/platform/win32/console_utils_win32.h"
#include "../../../include/utils/console_utils.h"

static bool FindMaxFontSize(HANDLE hConsole, int cols, int rows,
                            CONSOLE_FONT_INFOEX* outFont) { // ищет максимально возможный шрифт
    CONSOLE_FONT_INFOEX original; // хранит параметры шрифта
    original.cbSize = sizeof(original); // сохраняем оригинальный размер шрифта
    if (!GetCurrentConsoleFontEx(hConsole, FALSE, &original)) {
        return false; // если не получили данные шрифта
    }

    CONSOLE_FONT_INFOEX best = original; // храним лучший вариант
    bool found = false; // вернём true, если найдём подходящий вариант

    int lo = 5;
    int hi = 72;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2; // бинарный поиск максимально возможного шрифта

        CONSOLE_FONT_INFOEX test = original;
        test.dwFontSize.Y = static_cast<SHORT>(mid); // меняем высоту шрифта до mid
        test.dwFontSize.X = 0; // оставляем ширину шрифта пропорциональной высоте

        if (!SetCurrentConsoleFontEx(hConsole, FALSE, &test)) {
            hi = mid - 1;
            continue; // если шрифт слишком большой для применения, уменьшаем верхнюю границу
        }

        COORD largest = GetLargestConsoleWindowSize(hConsole); // получаем размер окна в символах
        if (largest.X >= cols && largest.Y >= rows) { // если рамка меньше окна, значит подходит
            best = test;
            found = true;
            lo = mid + 1;
        } else { // продолжаем поиски уменьшая ожидаемый резульатт
            hi = mid - 1;
        }
    }

    if (!found) { // не нашли, откатываем шрифт
        SetCurrentConsoleFontEx(hConsole, FALSE, &original);
        return false;
    }

    SetCurrentConsoleFontEx(hConsole, FALSE, &best); // меняем шрифт на найденный
    *outFont = best;
    return true;
}

bool console_utils::InitializeConsole(int cols, int rows) {
    if (cols <= 0 || rows <= 0) { // проверка на идиота
        return false;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // пытаемся получить хэндл вывода
    if (hConsole == INVALID_HANDLE_VALUE) {
        return false;
    }

    CONSOLE_FONT_INFOEX chosenFont;
    if (!FindMaxFontSize(hConsole, cols, rows, &chosenFont)) { // ищем размер шрифта написанной выше функцией
        return false;
    }

    if (!SetCurrentConsoleFontEx(hConsole, FALSE, &chosenFont)) { // устанавливаем шрифт второй раз, на случай не кооректной работы функции выше
        return false;
    }

    // Уменьшаем окно, чтобы буфер можно было свободно менять.
    SMALL_RECT minWin = { 0, 0, 0, 0 };
    if (!SetConsoleWindowInfo(hConsole, TRUE, &minWin)) {
        return false;
    }

    COORD largest = GetLargestConsoleWindowSize(hConsole); // смотрим максимальный размер окна в символах
    if (!SetConsoleScreenBufferSize(hConsole, largest)) { // растягиваем буфер
        return false;
    }

    SMALL_RECT maxWin = {
        0, 0,
        static_cast<SHORT>(largest.X - 1),
        static_cast<SHORT>(largest.Y - 1)
    };
    if (!SetConsoleWindowInfo(hConsole, TRUE, &maxWin)) { // растягиваем видимое окно до размеров буфера
        return false;
    }

    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER); // убираем заголовок, верхнюю большую рамку, тонкие рамки
        SetWindowLong(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // немедленно применяем новый стиль консоли

        ShowWindow(hwnd, SW_MAXIMIZE); // делаем на весь экран
        UpdateWindow(hwnd); // принудительно перерисовываем
    }

    // получаем актуальную информацию консоли после перерисовки
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
        }; // новый буфер либо как был раньше, либо стал больше
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
    // рисуем верхнюю линию
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

    for (int y = 1; y < rows; ++y) { // рисуем боковые части рамки
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
    for (int x = 0; x < cols; ++x) { // рисуем низ
        COORD posB = {
            static_cast<SHORT>(baseX + offsetX + x),
            static_cast<SHORT>(baseY + offsetY + rows - 1)
        };
        FillConsoleOutputCharacterA(hConsole, '-', 1, posB, &written);
    }

    COORD cornerBL = { // заменяем углы на плюсы внизу
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
