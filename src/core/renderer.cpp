#include "../../Include/core/renderer.h"
#include "../../Include/core/types.h"
#include "../../Include/core/sprites.h"
#include "../../Include/core/leaderboard.h"
#include "../../Include/core/menu.h"
#include "../../Include/core/save_system.h"
#include "../../Include/platform/win32/console_utils_win32.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Вспомогательная функция: запись символа и атрибута в буфер
static inline void write_cell(CHAR_INFO* buffer, int width, int x, int y, char c, WORD attr) {
    if (x < 0 || x >= width || y < 0) return;
    buffer[y * width + x].Char.AsciiChar = c;
    buffer[y * width + x].Attributes = attr;
}

// Статические буферы для двойной буферизации (в памяти, не консольные)
static CHAR_INFO* g_backBuffer = nullptr;
static int g_backBufferWidth = 0;
static int g_backBufferHeight = 0;

// Обновление шрифта консоли в зависимости от режима (меню/игра).
// Вызывать в начале кадра ДО renderer_clear.
// Меняет шрифт на ОБОИХ буферах, пересчитывает playWidth/playHeight/offsetX/offsetY.
void renderer_update_font(AppState* app) {
    // Определяем желаемый режим шрифта.
    // Крупный шрифт (FONT_MODE_MENU) — только для "настоящих" меню.
    // Пауза (MENU_PAUSE) и геймплей — мелкий шрифт (FONT_MODE_GAME).
    bool isLargeFontMenu = false;
    if (app->game.in_menu) {
        switch (app->menuState) {
            case MENU_MAIN:
            case MENU_HIGHSCORES:
            case MENU_SAVES:
            case MENU_CHANGE_NAME:
            case MENU_GAME_OVER:
                isLargeFontMenu = true;
                break;
            case MENU_PAUSE:
            case MENU_PLAY_NORMAL:
            case MENU_PLAY_DEFENSE:
            case MENU_EXIT:
            default:
                isLargeFontMenu = false;
                break;
        }
    }

    win32_console::FontMode desiredMode = isLargeFontMenu ? win32_console::FONT_MODE_MENU : win32_console::FONT_MODE_GAME;

    // Если режим не менялся — ничего не делаем
    if (app->currentFontMode == static_cast<uint8_t>(desiredMode)) {
        return;
    }

    // Меняем шрифт на ОБОИХ буферах двойной буферизации
    int newCols = 0, newRows = 0;
    bool ok0 = win32_console::SetConsoleFontMode(app->hBuffer[0], desiredMode, &newCols, &newRows);
    bool ok1 = win32_console::SetConsoleFontMode(app->hBuffer[1], desiredMode, &newCols, &newRows);

    if (!ok0 || !ok1) {
        // Не удалось сменить шрифт — остаёмся в старом режиме
        return;
    }

    // Обновляем размеры в AppState
    app->consoleWidth = newCols;
    app->consoleHeight = newRows;

    // Игровая область = окно минус рамка (2 символа по горизонтали и вертикали)
    int actualPlayWidth = newCols - 2;
    int actualPlayHeight = newRows - 2;
    if (actualPlayWidth < 10) actualPlayWidth = 10;
    if (actualPlayHeight < 10) actualPlayHeight = 10;

    app->offsetX = 1;
    app->offsetY = 1;
    app->playWidth = actualPlayWidth;
    app->playHeight = actualPlayHeight;

    // Синхронизируем GameState
    app->game.play_width = actualPlayWidth;
    app->game.play_height = actualPlayHeight;
    app->game.offset_x = 1;
    app->game.offset_y = 1;

    // Пересоздаём бэк-буфер в памяти под новый размер
    delete[] g_backBuffer;
    g_backBufferWidth = newCols;
    g_backBufferHeight = newRows;
    g_backBuffer = new CHAR_INFO[g_backBufferWidth * g_backBufferHeight];

    // Запоминаем текущий режим
    app->currentFontMode = static_cast<uint8_t>(desiredMode);
}

// Явная установка режима шрифта (вызывать ТОЛЬКО при смене состояния меню/игры).
// isLargeFont = true -> крупный шрифт для меню (MENU_MAIN, HIGHSCORES, SAVES, CHANGE_NAME, GAME_OVER)
// isLargeFont = false -> мелкий шрифт для игры и паузы
void renderer_set_font_mode(AppState* app, bool isLargeFont) {
    win32_console::FontMode desiredMode = isLargeFont ? win32_console::FONT_MODE_MENU : win32_console::FONT_MODE_GAME;

    // Если режим не менялся — ничего не делаем
    if (app->currentFontMode == static_cast<uint8_t>(desiredMode)) {
        return;
    }

    // Один консольный буфер — меняем шрифт на нём с ресайзом окна
    int newCols = 0, newRows = 0;
    bool ok = win32_console::SetConsoleFontMode(app->hConsole, desiredMode, &newCols, &newRows);

    if (!ok) {
        return;
    }

    // Обновляем размеры в AppState
    app->consoleWidth = newCols;
    app->consoleHeight = newRows;

    // Игровая область = окно минус рамка (2 символа по горизонтали и вертикали)
    int actualPlayWidth = newCols - 2;
    int actualPlayHeight = newRows - 2;
    if (actualPlayWidth < 10) actualPlayWidth = 10;
    if (actualPlayHeight < 10) actualPlayHeight = 10;

    app->offsetX = 1;
    app->offsetY = 1;
    app->playWidth = actualPlayWidth;
    app->playHeight = actualPlayHeight;

    // Синхронизируем GameState
    app->game.play_width = actualPlayWidth;
    app->game.play_height = actualPlayHeight;
    app->game.offset_x = 1;
    app->game.offset_y = 1;

    // Пересоздаём бэк-буфер в памяти под новый размер
    delete[] g_backBuffer;
    g_backBufferWidth = newCols;
    g_backBufferHeight = newRows;
    g_backBuffer = new CHAR_INFO[g_backBufferWidth * g_backBufferHeight];

    // Запоминаем текущий режим
    app->currentFontMode = static_cast<uint8_t>(desiredMode);
}

// === Font Sprite System ===

// Загрузка шрифта из файла (96 символов 32..127, каждый 6x6)
bool font_load_from_file(AppState* app, const char* filename) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    FontAtlas* font = &app->game.font;
    DWORD read;
    BOOL result = ReadFile(hFile, font->chars, sizeof(FontChar) * FONT_TOTAL_CHARS, &read, NULL);
    CloseHandle(hFile);

    if (!result || read != sizeof(FontChar) * FONT_TOTAL_CHARS) return false;
    font->loaded = 1;
    return true;
}

// Отрисовка одного символа через спрайт шрифта (6x6)
void font_draw_char(AppState* app, int x, int y, char c, WORD color) {
    FontAtlas* font = &app->game.font;
    if (!font->loaded) return;
    unsigned char uc = static_cast<unsigned char>(c);
    if (uc < 32 || uc > 127) return;

    const FontChar* fc = &font->chars[uc - 32];
    int bufX = app->offsetX + x;
    int bufY = app->offsetY + y;

    for (int row = 0; row < FONT_CHAR_H; ++row) {
        int drawY = bufY + row;
        if (drawY < 0 || drawY >= g_backBufferHeight) continue;
        for (int col = 0; col < FONT_CHAR_W; ++col) {
            char ch = fc->data[row][col];
            if (ch != ' ') {
                int drawX = bufX + col;
                if (drawX >= 0 && drawX < g_backBufferWidth) {
                    write_cell(g_backBuffer, g_backBufferWidth, drawX, drawY, ch, color);
                }
            }
        }
    }
}

// Отрисовка строки через спрайт шрифта (с фоллбэком на обычный текст)
void font_draw_str(AppState* app, int x, int y, const char* str, WORD color) {
    FontAtlas* font = &app->game.font;
    if (font->loaded) {
        int cursorX = x;
        for (int i = 0; str[i]; ++i) {
            font_draw_char(app, cursorX, y, str[i], color);
            cursorX += FONT_CHAR_W + 1;
        }
    } else {
        // Фоллбэк: обычный текст через renderer_draw_str
        renderer_draw_str(app, x, y, str, color);
    }
}

// Отрисовка числа через спрайт шрифта (с фоллбэком)
void font_draw_int(AppState* app, int x, int y, int value, WORD color) {
    char buf[32];
    _itoa_s(value, buf, 10);
    font_draw_str(app, x, y, buf, color);
}

// Отрисовка меню через спрайт шрифта (с фоллбэком)
void font_draw_menu(AppState* app, int x, int y, const char* items[], int count, int selected, WORD normal_color, WORD selected_color) {
    FontAtlas* font = &app->game.font;
    int lineHeight = font->loaded ? FONT_CHAR_H : 1; // обычный текст = 1 строка
    for (int i = 0; i < count; ++i) {
        WORD color = (i == selected) ? selected_color : normal_color;
        font_draw_str(app, x, y + i * lineHeight, items[i], color);
    }
}

// ============================================================

bool renderer_init(AppState* app) {
    // Используем ТОЛЬКО ОДИН консольный буфер (hConsole).
    // Двойная буферизация делается в памяти (g_backBuffer).
    app->hBuffer[0] = app->hConsole;
    app->hBuffer[1] = nullptr; // не используется
    app->activeBuffer = 0;

    // Скрываем курсор
    CONSOLE_CURSOR_INFO cursorInfo = {1, FALSE};
    SetConsoleCursorInfo(app->hConsole, &cursorInfo);

    // Выделяем системный бэк-буфер в памяти
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(app->hConsole, &info);
    g_backBufferWidth = info.dwSize.X;
    g_backBufferHeight = info.dwSize.Y;
    g_backBuffer = new CHAR_INFO[g_backBufferWidth * g_backBufferHeight];

    // Загружаем шрифт - пробуем несколько путей
    if (!font_load_from_file(app, "assets/sprites/font.bin")) {
        if (!font_load_from_file(app, "../assets/sprites/font.bin")) {
            font_load_from_file(app, "../../assets/sprites/font.bin");
        }
    }

    return true;
}

void renderer_clear(AppState* /*app*/, WORD color) {
    int total = g_backBufferWidth * g_backBufferHeight;
    for (int i = 0; i < total; ++i) {
        g_backBuffer[i].Char.AsciiChar = ' ';
        g_backBuffer[i].Attributes = color;
    }
}

CHAR_INFO* renderer_get_buffer(AppState* /*app*/) {
    return g_backBuffer;
}

void renderer_draw_char(AppState* app, int x, int y, char c, WORD color) {
    // Обычный текст (для спрайтов игры)
    int bufX = app->offsetX + x;
    int bufY = app->offsetY + y;

    if (bufX >= 0 && bufX < g_backBufferWidth && bufY >= 0 && bufY < g_backBufferHeight) {
        write_cell(g_backBuffer, g_backBufferWidth, bufX, bufY, c, color);
    }
}

void renderer_draw_str(AppState* app, int x, int y, const char* str, WORD color) {
    int bufX = app->offsetX + x;
    int bufY = app->offsetY + y;

    if (bufY < 0 || bufY >= g_backBufferHeight) return;

    for (int i = 0; str[i] && bufX + i < g_backBufferWidth; ++i) {
        write_cell(g_backBuffer, g_backBufferWidth, bufX + i, bufY, str[i], color);
    }
}

void renderer_draw_sprite(AppState* app, int x, int y, const char* sprite[], int lines, WORD color) {
    for (int i = 0; i < lines; ++i) {
        if (sprite[i]) {
            renderer_draw_str(app, x, y + i, sprite[i], color);
        }
    }
}

void renderer_draw_sprite_transparent(AppState* app, int x, int y, const char* sprite[], int lines, WORD color, char transparent_char) {
    for (int i = 0; i < lines; ++i) {
        if (!sprite[i]) continue;
        const char* line = sprite[i];
        int bufY = app->offsetY + y + i;
        if (bufY < 0 || bufY >= g_backBufferHeight) continue;

        for (int j = 0; line[j]; ++j) {
            if (line[j] != transparent_char) {
                int bufX = app->offsetX + x + j;
                if (bufX >= 0 && bufX < g_backBufferWidth) {
                    write_cell(g_backBuffer, g_backBufferWidth, bufX, bufY, line[j], color);
                }
            }
        }
    }
}

void renderer_draw_frame(AppState* app) {
    int left = app->offsetX;
    int right = app->offsetX + app->playWidth - 1;
    int top = app->offsetY;
    int bottom = app->offsetY + app->playHeight - 1;

    WORD frameColor = COL_FRAME;

    // Верх и низ
    for (int x = left; x <= right; ++x) {
        if (top >= 0 && top < g_backBufferHeight) write_cell(g_backBuffer, g_backBufferWidth, x, top, '-', frameColor);
        if (bottom >= 0 && bottom < g_backBufferHeight) write_cell(g_backBuffer, g_backBufferWidth, x, bottom, '-', frameColor);
    }

    // Стороны
    for (int y = top; y <= bottom; ++y) {
        if (y >= 0 && y < g_backBufferHeight) {
            write_cell(g_backBuffer, g_backBufferWidth, left, y, '|', frameColor);
            write_cell(g_backBuffer, g_backBufferWidth, right, y, '|', frameColor);
        }
    }

    // Углы
    if (top >= 0 && top < g_backBufferHeight && left >= 0 && left < g_backBufferWidth)
        write_cell(g_backBuffer, g_backBufferWidth, left, top, '+', frameColor);
    if (top >= 0 && top < g_backBufferHeight && right >= 0 && right < g_backBufferWidth)
        write_cell(g_backBuffer, g_backBufferWidth, right, top, '+', frameColor);
    if (bottom >= 0 && bottom < g_backBufferHeight && left >= 0 && left < g_backBufferWidth)
        write_cell(g_backBuffer, g_backBufferWidth, left, bottom, '+', frameColor);
    if (bottom >= 0 && bottom < g_backBufferHeight && right >= 0 && right < g_backBufferWidth)
        write_cell(g_backBuffer, g_backBufferWidth, right, bottom, '+', frameColor);
}

void renderer_draw_player(AppState* app) {
    GameState* gs = &app->game;
    Player* p = &gs->player;

    int spriteX = p->x - SPRITE_PLAYER_W / 2;
    int spriteY = p->y - SPRITE_PLAYER_H + 1;

    renderer_draw_sprite_transparent(app, spriteX, spriteY, SPRITE_PLAYER, SPRITE_PLAYER_H, COL_PLAYER, ' ');

    if (p->shield_active) {
        int shieldX = spriteX - p->shield_hitbox_extra;
        int shieldY = spriteY - p->shield_hitbox_extra;
        int shieldW = SPRITE_PLAYER_W + 2 * p->shield_hitbox_extra;
        int shieldH = SPRITE_PLAYER_H + 2 * p->shield_hitbox_extra;

        WORD shieldColor = COL_CYAN | (COL_BLACK << 4);
        for (int i = 0; i < shieldW; ++i) {
            renderer_draw_char(app, shieldX + i, shieldY, '#', shieldColor);
            renderer_draw_char(app, shieldX + i, shieldY + shieldH - 1, '#', shieldColor);
        }
        for (int i = 0; i < shieldH; ++i) {
            renderer_draw_char(app, shieldX, shieldY + i, '#', shieldColor);
            renderer_draw_char(app, shieldX + shieldW - 1, shieldY + i, '#', shieldColor);
        }
    }
}

void renderer_draw_enemies(AppState* app) {
    GameState* gs = &app->game;

    for (int i = 0; i < gs->enemy_count; ++i) {
        Enemy* e = &gs->enemies[i];
        if (!e->active) continue;

        int spriteX = e->x - SPRITE_ENEMY_W / 2;
        int spriteY = e->y - SPRITE_ENEMY_H / 2;

        renderer_draw_sprite_transparent(app, spriteX, spriteY, SPRITE_ENEMY, SPRITE_ENEMY_H, COL_ENEMY, ' ');

        // Зелёная часть: рисуем от ТОЙ ЖЕ точки (spriteX, spriteY), что и основной спрайт.
        // В SPRITE_ENEMY_GREEN первые 2 строки пустые (""), поэтому контент (глаза в строке 2)
        // идеально совпадает с основным спрайтом при одинаковой позиции левого верхнего угла.
        renderer_draw_sprite_transparent(app, spriteX, spriteY, SPRITE_ENEMY_GREEN, SPRITE_ENEMY_GREEN_H, COL_ENEMY_GREEN, ' ');
    }
}

void renderer_draw_bullets(AppState* app) {
    GameState* gs = &app->game;

    for (int i = 0; i < gs->bullet_up_count; ++i) {
        Bullet* b = &gs->bullets_up[i];
        if (b->active) {
            renderer_draw_char(app, b->x, b->y, SPRITE_BULLET, COL_BULLET);
        }
    }

    for (int i = 0; i < gs->bullet_ur_count; ++i) {
        Bullet* b = &gs->bullets_ur[i];
        if (b->active) {
            renderer_draw_char(app, b->x, b->y, '/', COL_BULLET);
        }
    }

    for (int i = 0; i < gs->bullet_ul_count; ++i) {
        Bullet* b = &gs->bullets_ul[i];
        if (b->active) {
            renderer_draw_char(app, b->x, b->y, '\\', COL_BULLET);
        }
    }
}

void renderer_draw_powerups(AppState* app) {
    GameState* gs = &app->game;

    for (int i = 0; i < gs->powerup_count; ++i) {
        PowerUp* p = &gs->powerups[i];
        if (!p->active) continue;

        const char** sprite = nullptr;
        switch (p->type) {
            case POWERUP_SHIELD:
                sprite = SPRITE_POWERUP_SHIELD;
                break;
            case POWERUP_TRIPLE_STRAIGHT:
                sprite = SPRITE_POWERUP_TRIPLE_STRAIGHT;
                break;
            case POWERUP_TRIPLE_DIAGONAL:
                sprite = SPRITE_POWERUP_TRIPLE_DIAGONAL;
                break;
        }

        if (sprite) {
            renderer_draw_sprite_transparent(app, p->x - 1, p->y - 1, sprite, SPRITE_POWERUP_H, COL_POWERUP, ' ');
        }
    }
}

void renderer_draw_ui(AppState* app) {
    GameState* gs = &app->game;

    // Обычный текст — 1 символ = 1 ячейка. play_width ~300 в игровом режиме.
    renderer_draw_str(app, 2, 1, "SCORE:", COL_UI_TEXT);
    char scoreBuf[32];
    _itoa_s(gs->player.score, scoreBuf, 10);
    renderer_draw_str(app, 9, 1, scoreBuf, COL_UI_YELLOW);

    renderer_draw_str(app, gs->play_width - 14, 1, "LIVES:", COL_UI_TEXT);
    char hpBuf[16];
    _itoa_s(gs->player.hp, hpBuf, 10);
    renderer_draw_str(app, gs->play_width - 7, 1, hpBuf, COL_UI_RED);

    if (gs->mode == MODE_DEFENSE) {
        int baseX = gs->play_width / 2 - 10;
        renderer_draw_str(app, baseX, 1, "BASE:", COL_UI_TEXT);
        char baseHpBuf[16], baseMaxBuf[16];
        _itoa_s(gs->base_hp, baseHpBuf, 10);
        _itoa_s(gs->base_max_hp, baseMaxBuf, 10);
        renderer_draw_str(app, baseX + 6, 1, baseHpBuf, COL_UI_YELLOW);
        renderer_draw_str(app, baseX + 6 + static_cast<int>(strlen(baseHpBuf)), 1, "/", COL_UI_TEXT);
        renderer_draw_str(app, baseX + 6 + static_cast<int>(strlen(baseHpBuf)) + 1, 1, baseMaxBuf, COL_UI_GREEN);
    }

    int y = 2;
    if (gs->player.shield_active) {
        renderer_draw_str(app, 2, y, "SHIELD", COL_POWERUP);
        y++;
    }
    if (gs->player.triple_straight_timer > 0) {
        renderer_draw_str(app, 2, y, "TRIPLE: ", COL_POWERUP);
        char tBuf[16];
        _itoa_s(gs->player.triple_straight_timer / 60, tBuf, 10);
        renderer_draw_str(app, 10, y, tBuf, COL_YELLOW);
        y++;
    }
    if (gs->player.triple_diagonal_timer > 0) {
        renderer_draw_str(app, 2, y, "DIAG: ", COL_POWERUP);
        char dBuf[16];
        _itoa_s(gs->player.triple_diagonal_timer / 60, dBuf, 10);
        renderer_draw_str(app, 9, y, dBuf, COL_YELLOW);
        y++;
    }
}

void renderer_draw_menu(AppState* app) {
    renderer_clear(app, COL_MENU_BG);
    renderer_draw_frame(app);

    const char* items[MAIN_COUNT] = {
        "PLAY NORMAL",
        "PLAY DEFENSE",
        "HIGH SCORES",
        "SAVES",
        "CHANGE NAME",
        "EXIT GAME"
    };

    // Обычный текст — 1 символ = 1 ячейка. Центрируем по playWidth.
    int titleX = app->playWidth / 2 - 10; // "=== SPACE INVADERS ===" = 21 символ
    renderer_draw_str(app, titleX, 2, "=== SPACE INVADERS ===", COL_MENU_SEL);

    int nameX = app->playWidth / 2 - static_cast<int>(strlen(app->playerName)) / 2;
    renderer_draw_str(app, nameX, 4, app->playerName, COL_UI_YELLOW);

    // Пункты меню — центрированные, по 1 строке на пункт
    int startY = app->playHeight / 2 - MAIN_COUNT / 2;

    for (int i = 0; i < MAIN_COUNT; ++i) {
        WORD color = (i == app->menuSelection) ? COL_MENU_SEL : COL_MENU_NORM;
        int itemX = app->playWidth / 2 - static_cast<int>(strlen(items[i])) / 2;
        renderer_draw_str(app, itemX, startY + i, items[i], color);
    }

    const char* hint = "W/S or Arrows to navigate, Enter to select";
    int hintX = app->playWidth / 2 - static_cast<int>(strlen(hint)) / 2;
    renderer_draw_str(app, hintX, startY + MAIN_COUNT + 1, hint, COL_UI_TEXT);
}

void renderer_draw_pause(AppState* app) {
    const char* items[PAUSE_COUNT] = {
        "RESUME",
        "MAIN MENU",
        "EXIT GAME"
    };

    renderer_draw_str(app, app->playWidth / 2 - 7, app->playHeight / 2 - 4, "=== PAUSED ===", COL_MENU_SEL);

    int startY = app->playHeight / 2 - PAUSE_COUNT / 2;
    for (int i = 0; i < PAUSE_COUNT; ++i) {
        WORD color = (i == app->menuSelection) ? COL_MENU_SEL : COL_UI_TEXT;
        int itemX = app->playWidth / 2 - static_cast<int>(strlen(items[i])) / 2;
        renderer_draw_str(app, itemX, startY + i, items[i], color);
    }
}

void renderer_draw_game_over(AppState* app) {
    GameState* gs = &app->game;
    renderer_clear(app, COL_GAME_BG);
    renderer_draw_frame(app);

    renderer_draw_str(app, app->playWidth / 2 - 4, app->playHeight / 2 - 5, "GAME OVER", COL_UI_RED);

    const char* scoreLabel = "FINAL SCORE:";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(scoreLabel)) / 2, app->playHeight / 2 - 3, scoreLabel, COL_UI_TEXT);
    char scoreBuf[32];
    _itoa_s(gs->player.score, scoreBuf, 10);
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(scoreBuf)) / 2, app->playHeight / 2 - 2, scoreBuf, COL_UI_YELLOW);

    const char* items[GO_COUNT] = {
        "RESTART",
        "MAIN MENU",
        "EXIT"
    };

    int startY = app->playHeight / 2;
    for (int i = 0; i < GO_COUNT; ++i) {
        WORD color = (i == app->menuSelection) ? COL_MENU_SEL : COL_UI_TEXT;
        int itemX = app->playWidth / 2 - static_cast<int>(strlen(items[i])) / 2;
        renderer_draw_str(app, itemX, startY + i, items[i], color);
    }
}

void renderer_draw_leaderboard(AppState* app, GameMode mode) {
    renderer_clear(app, COL_MENU_BG);
    renderer_draw_frame(app);

    const char* modeName = (mode == MODE_NORMAL) ? "NORMAL MODE" : "DEFENSE MODE";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(modeName)) / 2, 2, modeName, COL_MENU_SEL);

    const char* header = "RANK  NAME                    SCORE";
    renderer_draw_str(app, 4, 4, header, COL_UI_TEXT);

    LeaderboardEntry entries[MAX_LEADERBOARD];
    int count = leaderboard_get(app, mode, entries, MAX_LEADERBOARD);

    for (int i = 0; i < count; ++i) {
        int y = 6 + i;
        char rankBuf[16];
        _itoa_s(i + 1, rankBuf, 10);
        char scoreBuf[32];
        _itoa_s(entries[i].score, scoreBuf, 10);

        // Формируем строку: "  1.  PlayerName              12345"
        char line[128];
        _snprintf_s(line, sizeof(line), _TRUNCATE, "%2d.  %-24s  %s", i + 1, entries[i].name, scoreBuf);
        renderer_draw_str(app, 4, y, line, (i == app->menuSelection) ? COL_MENU_SEL : COL_UI_TEXT);
    }

    if (count == 0) {
        renderer_draw_str(app, app->playWidth / 2 - 5, app->playHeight / 2, "NO SCORES YET", COL_UI_TEXT);
    }

    const char* hint = "PRESS ESC TO RETURN";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(hint)) / 2, app->playHeight - 3, hint, COL_UI_TEXT);
}

void renderer_draw_saves(AppState* app) {
    renderer_clear(app, COL_MENU_BG);
    renderer_draw_frame(app);

    const char* title = "SAVED GAMES";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(title)) / 2, 2, title, COL_MENU_SEL);

    SaveSlot saves[MAX_SAVES];
    int count = saves_get_list(app, saves, MAX_SAVES);

    if (count == 0) {
        const char* msg = "NO SAVES FOUND";
        renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(msg)) / 2, app->playHeight / 2, msg, COL_UI_TEXT);
    } else {
        const char* header = "SEL  DATE       TIME     MODE     SCORE";
        renderer_draw_str(app, 4, 4, header, COL_UI_TEXT);

        for (int i = 0; i < count; ++i) {
            FILETIME ft;
            ft.dwLowDateTime = static_cast<DWORD>(saves[i].timestamp & 0xFFFFFFFF);
            ft.dwHighDateTime = static_cast<DWORD>(saves[i].timestamp >> 32);

            SYSTEMTIME st;
            FileTimeToSystemTime(&ft, &st);

            char buffer[128];
            const char* modeStr = (saves[i].mode == MODE_NORMAL) ? "NORMAL" : "DEFENSE";
            _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%2d  %02d.%02d.%04d %02d:%02d  %-6s  %u",
                i, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, modeStr, saves[i].player.score);

            WORD color = (i == app->menuSelection) ? COL_MENU_SEL : COL_UI_TEXT;
            int y = 6 + i;
            renderer_draw_str(app, 4, y, buffer, color);
        }
    }

    const char* hint = "ENTER: LOAD  DEL: DELETE  ESC: BACK";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(hint)) / 2, app->playHeight - 3, hint, COL_UI_TEXT);
}

extern char g_nameInput[MAX_NAME_LEN];
extern int g_nameInputLen;

void renderer_draw_change_name(AppState* app) {
    renderer_clear(app, COL_MENU_BG);
    renderer_draw_frame(app);

    int centerY = app->playHeight / 2;

    const char* title = "CHANGE NAME";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(title)) / 2, centerY - 4, title, COL_MENU_SEL);

    const char* currLabel = "CURRENT: ";
    renderer_draw_str(app, app->playWidth / 2 - 10, centerY - 2, currLabel, COL_UI_TEXT);
    renderer_draw_str(app, app->playWidth / 2 + 1, centerY - 2, app->playerName, COL_MENU_SEL);

    const char* newLabel = "NEW: ";
    renderer_draw_str(app, app->playWidth / 2 - 10, centerY, newLabel, COL_UI_TEXT);
    renderer_draw_str(app, app->playWidth / 2 + 1, centerY, g_nameInput, COL_UI_TEXT);

    // Курсор подчеркиванием
    int cursorX = app->playWidth / 2 + 1 + g_nameInputLen;
    renderer_draw_char(app, cursorX, centerY, '_', COL_MENU_SEL);

    const char* hint = "TYPE NAME AND PRESS ENTER, ESC TO CANCEL";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(hint)) / 2, centerY + 3, hint, COL_UI_TEXT);
}

void renderer_draw_save_preview(AppState* app) {
    renderer_clear(app, COL_GAME_BG);
    renderer_draw_frame(app);
    renderer_draw_player(app);
    renderer_draw_enemies(app);
    renderer_draw_bullets(app);
    renderer_draw_powerups(app);
    renderer_draw_ui(app);

    const char* msg = "PRESS ANY KEY TO RESUME...";
    renderer_draw_str(app, app->playWidth / 2 - static_cast<int>(strlen(msg)) / 2, app->playHeight / 2, msg, COL_MENU_SEL);
}

void renderer_present(AppState* app) {
    // Один буфер — без переключения
    HANDLE hBuf = app->hConsole;

    COORD bufSize = { static_cast<SHORT>(g_backBufferWidth), static_cast<SHORT>(g_backBufferHeight) };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, static_cast<SHORT>(g_backBufferWidth - 1), static_cast<SHORT>(g_backBufferHeight - 1) };

    WriteConsoleOutputA(hBuf, g_backBuffer, bufSize, bufCoord, &writeRegion);
}