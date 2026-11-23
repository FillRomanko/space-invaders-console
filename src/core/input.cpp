#include "../../Include/core/input.h"
#include "../../Include/core/types.h"
#include <windows.h>
#include <cstring>

// Статический массив для отслеживания удержанных клавиш между кадрами
static uint8_t g_key_held[KEY_COUNT] = {0};

bool input_init(AppState* app) {
    app->hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (app->hInput == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Включаем режим мыши и окна
    DWORD mode;
    GetConsoleMode(app->hInput, &mode);
    mode |= ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE; // Отключаем QuickEdit для захвата мыши
    SetConsoleMode(app->hInput, mode);

    // Обнуляем удержанные клавиши
    memset(g_key_held, 0, sizeof(g_key_held));

    return true;
}

void input_poll(AppState* app, InputState* out) {
    // Обнуляем состояние нажатий текущего кадра
    memset(out->key_pressed, 0, sizeof(out->key_pressed));
    out->mouse_left = 0;
    out->mouse_right = 0;
    out->mouse_moved = 0;
    out->input_char_count = 0;

    // Неблокирующий опрос: сначала Peek, чтобы узнать есть ли события
    DWORD numEvents = 0;
    if (!PeekConsoleInputA(app->hInput, app->inputBuffer, 128, &numEvents)) {
        // Ошибка — копируем удержанное состояние и выходим
        memcpy(out->key_held, g_key_held, sizeof(g_key_held));
        return;
    }

    if (numEvents == 0) {
        // Событий нет — просто копируем удержанное состояние
        memcpy(out->key_held, g_key_held, sizeof(g_key_held));
        return;
    }

    // Есть события — читаем их ReadConsoleInputA (теперь оно не заблокируется)
    INPUT_RECORD rec;
    DWORD numRead = 0;

    while (numEvents > 0 && ReadConsoleInputA(app->hInput, &rec, 1, &numRead) && numRead > 0) {
        numEvents--;
        switch (rec.EventType) {
            case KEY_EVENT: {
                WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
                if (vk >= KEY_COUNT) break;

                if (rec.Event.KeyEvent.bKeyDown) {
                    // Клавиша нажата (или автоповтор при удержании).
                    // key_pressed = 1 на каждом такте автоповтора — это даёт
                    // возможность зажать пробел для стрельбы.
                    out->key_pressed[vk] = 1;
                    g_key_held[vk] = 1;
                } else {
                    // Клавиша отпущена
                    g_key_held[vk] = 0;
                }

                // Обработка текстового ввода (UnicodeChar)
                WCHAR ch = rec.Event.KeyEvent.uChar.UnicodeChar;
                if (ch != 0 && ch != '\r' && ch != '\n' && ch != '\b' && ch != '\t' && ch != 27) {
                    // Печатаемые символы (ASCII диапазон)
                    if (ch >= 32 && ch <= 126 && out->input_char_count < 16) {
                        out->input_chars[out->input_char_count++] = static_cast<char>(ch);
                    }
                }
                break;
            }
            case MOUSE_EVENT: {
                out->mouse_x = rec.Event.MouseEvent.dwMousePosition.X;
                out->mouse_y = rec.Event.MouseEvent.dwMousePosition.Y;

                // Конвертируем в координаты игрового поля
                out->mouse_x -= app->offsetX;
                out->mouse_y -= app->offsetY;

                if (rec.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
                    out->mouse_left = 1;
                }
                if (rec.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
                    out->mouse_right = 1;
                }
                if (rec.Event.MouseEvent.dwEventFlags & MOUSE_MOVED) {
                    out->mouse_moved = 1;
                }
                break;
            }
            case WINDOW_BUFFER_SIZE_EVENT: {
                // Окно изменило размер - обновим буфер
                CONSOLE_SCREEN_BUFFER_INFO info;
                GetConsoleScreenBufferInfo(app->hConsole, &info);
                app->bufferInfo = info;
                app->consoleWidth = info.srWindow.Right - info.srWindow.Left + 1;
                app->consoleHeight = info.srWindow.Bottom - info.srWindow.Top + 1;
                break;
            }
            // Остальные типы событий (FOCUS_EVENT, MENU_EVENT) игнорируем
        }
    }

    // Копируем текущее состояние удержания
    memcpy(out->key_held, g_key_held, sizeof(g_key_held));
}

uint8_t input_is_pressed(const InputState* state, KeyCode key) {
    if (key >= KEY_COUNT) return 0;
    return state->key_pressed[key];
}

uint8_t input_is_held(const InputState* state, KeyCode key) {
    if (key >= KEY_COUNT) return 0;
    return state->key_held[key];
}

void input_update_held_state(const InputState* current, InputState* held_state) {
    // Эта функция больше не нужна - состояние хранится в g_key_held
    (void)current;
    (void)held_state;
}