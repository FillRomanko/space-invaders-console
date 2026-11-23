#include "../../Include/core/renderer.h"
#include "../../Include/core/types.h"
#include "../../Include/core/sprites.h"
#include "../../Include/core/leaderboard.h"
#include "../../Include/core/menu.h"
#include "../../Include/core/save_system.h"
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
    // Создаем второй консольный буфер для двойной буферизации
    app->hBuffer[0] = app->hConsole;
    app->hBuffer[1] = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    if (app->hBuffer[1] == INVALID_HANDLE_VALUE) {
        return false;
    }

    app->activeBuffer = 0;

    // Скрываем курсор в обоих буферах
    CONSOLE_CURSOR_INFO cursorInfo = {1, FALSE};
    SetConsoleCursorInfo(app->hBuffer[0], &cursorInfo);
    SetConsoleCursorInfo(app->hBuffer[1], &cursorInfo);

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

        // Зелёная часть: центрируем по своей высоте, чтобы контент (глаза в строке 2) совпадал
        // Основной спрайт: глаза в строке 2, всего 17 строк, центр в 8.5 -> верхушка в y-8
        // Зелёный спрайт: глаза в строке 2, всего 14 строк, центр в 7 -> верхушка в y-7
        // Чтобы глаза совпали: greenY = e->y - 7 = e->y - SPRITE_ENEMY_GREEN_H / 2
        int greenX = e->x - SPRITE_ENEMY_GREEN_W / 2;
        int greenY = e->y - SPRITE_ENEMY_GREEN_H / 2;
        renderer_draw_sprite_transparent(app, greenX, greenY, SPRITE_ENEMY_GREEN, SPRITE_ENEMY_GREEN_H, COL_ENEMY_GREEN, ' ');
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

    font_draw_str(app, 2, 1, "SCORE:", COL_UI_TEXT);
    font_draw_int(app, 2 + 7 * FONT_CHAR_W, 1, gs->player.score, COL_UI_YELLOW);

    font_draw_str(app, gs->play_width - 12 * FONT_CHAR_W, 1, "LIVES:", COL_UI_TEXT);
    font_draw_int(app, gs->play_width - 5 * FONT_CHAR_W, 1, gs->player.hp, COL_UI_RED);

    if (gs->mode == MODE_DEFENSE) {
        int baseX = gs->play_width / 2 - 8 * FONT_CHAR_W;
        font_draw_str(app, baseX, 1, "BASE:", COL_UI_TEXT);
        font_draw_int(app, baseX + 6 * FONT_CHAR_W, 1, gs->base_hp, COL_UI_YELLOW);
        font_draw_char(app, baseX + 8 * FONT_CHAR_W, 1, '/', COL_UI_TEXT);
        font_draw_int(app, baseX + 9 * FONT_CHAR_W, 1, gs->base_max_hp, COL_UI_GREEN);
    }

    int y = 1 + FONT_CHAR_H;
    if (gs->player.shield_active) {
        font_draw_str(app, 2, y, "SHIELD", COL_POWERUP);
        y += FONT_CHAR_H;
    }
    if (gs->player.triple_straight_timer > 0) {
        font_draw_str(app, 2, y, "TRIPLE: ", COL_POWERUP);
        font_draw_int(app, 2 + 8 * FONT_CHAR_W, y, gs->player.triple_straight_timer / 60, COL_YELLOW);
        y += FONT_CHAR_H;
    }
    if (gs->player.triple_diagonal_timer > 0) {
        font_draw_str(app, 2, y, "DIAG: ", COL_POWERUP);
        font_draw_int(app, 2 + 6 * FONT_CHAR_W, y, gs->player.triple_diagonal_timer / 60, COL_YELLOW);
        y += FONT_CHAR_H;
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

    font_draw_str(app, app->playWidth / 2 - 12 * FONT_CHAR_W, 3, "=== SPACE INVADERS ===", COL_MENU_SEL);

    font_draw_str(app, app->playWidth / 2 - 10 * FONT_CHAR_W, 4 + FONT_CHAR_H, app->playerName, COL_UI_YELLOW);

    int startY = app->playHeight / 2 - MAIN_COUNT / 2 * (FONT_CHAR_H + 1);
    int startX = app->playWidth / 2 - 10 * FONT_CHAR_W;

    font_draw_menu(app, startX, startY, items, MAIN_COUNT, app->menuSelection, COL_MENU_NORM, COL_MENU_SEL);

    font_draw_str(app, app->playWidth / 2 - 18 * FONT_CHAR_W, startY + MAIN_COUNT * 2 * (FONT_CHAR_H + 1) + (FONT_CHAR_H + 1),
        "W/S or Arrows to navigate, Enter to select", COL_UI_TEXT);
}

void renderer_draw_pause(AppState* app) {
    const char* items[PAUSE_COUNT] = {
        "RESUME",
        "MAIN MENU",
        "EXIT GAME"
    };

    font_draw_str(app, app->playWidth / 2 - 6 * FONT_CHAR_W, app->playHeight / 2 - 3 * (FONT_CHAR_H + 1), "=== PAUSED ===", COL_MENU_SEL);

    int startX = app->playWidth / 2 - 7 * FONT_CHAR_W;
    int startY = app->playHeight / 2 - (FONT_CHAR_H + 1);
    font_draw_menu(app, startX, startY, items, PAUSE_COUNT, app->menuSelection, COL_UI_TEXT, COL_MENU_SEL);
}

void renderer_draw_game_over(AppState* app) {
    GameState* gs = &app->game;
    renderer_clear(app, COL_GAME_BG);
    renderer_draw_frame(app);

    font_draw_str(app, app->playWidth / 2 - 5 * FONT_CHAR_W, app->playHeight / 2 - 4 * (FONT_CHAR_H + 1), "GAME OVER", COL_UI_RED);

    font_draw_str(app, app->playWidth / 2 - 10 * FONT_CHAR_W, app->playHeight / 2 - 2 * (FONT_CHAR_H + 1), "FINAL SCORE:", COL_UI_TEXT);
    char scoreBuf[32];
    _itoa_s(gs->player.score, scoreBuf, 10);
    font_draw_str(app, app->playWidth / 2 + 2 * FONT_CHAR_W, app->playHeight / 2 - 2 * (FONT_CHAR_H + 1), scoreBuf, COL_UI_YELLOW);

    const char* items[GO_COUNT] = {
        "RESTART",
        "MAIN MENU",
        "EXIT"
    };

    int startX = app->playWidth / 2 - 5 * FONT_CHAR_W;
    int startY = app->playHeight / 2;
    font_draw_menu(app, startX, startY, items, GO_COUNT, app->menuSelection, COL_UI_TEXT, COL_MENU_SEL);
}

void renderer_draw_leaderboard(AppState* app, GameMode mode) {
    renderer_clear(app, COL_MENU_BG);
    renderer_draw_frame(app);

    const char* modeName = (mode == MODE_NORMAL) ? "NORMAL MODE" : "DEFENSE MODE";
    font_draw_str(app, app->playWidth / 2 - 5 * FONT_CHAR_W, 2, modeName, COL_MENU_SEL);

    font_draw_str(app, 4, 4 + FONT_CHAR_H + 1, "RANK  NAME                    SCORE", COL_UI_TEXT);

    LeaderboardEntry entries[MAX_LEADERBOARD];
    int count = leaderboard_get(app, mode, entries, MAX_LEADERBOARD);

    for (int i = 0; i < count; ++i) {
        int y = 6 + i * (FONT_CHAR_H + 1);
        font_draw_str(app, 4, y, "  ", COL_UI_TEXT);
        char rankBuf[16];
        _itoa_s(i + 1, rankBuf, 10);
        font_draw_str(app, 4, y, rankBuf, COL_UI_YELLOW);
        font_draw_str(app, 4 + 4 * FONT_CHAR_W, y, ".  ", COL_UI_TEXT);
        font_draw_str(app, 4 + 7 * FONT_CHAR_W, y, entries[i].name, COL_UI_TEXT);
        char scoreBuf[32];
        _itoa_s(entries[i].score, scoreBuf, 10);
        font_draw_str(app, app->playWidth - 12 * FONT_CHAR_W, y, scoreBuf, COL_UI_YELLOW);
    }

    if (count == 0) {
        font_draw_str(app, app->playWidth / 2 - 5 * FONT_CHAR_W, app->playHeight / 2, "NO SCORES YET", COL_UI_TEXT);
    }

    font_draw_str(app, app->playWidth / 2 - 9 * FONT_CHAR_W, app->playHeight - 3 - (FONT_CHAR_H + 1), "PRESS ESC TO RETURN", COL_UI_TEXT);
}

void renderer_draw_saves(AppState* app) {
    renderer_clear(app, COL_MENU_BG);
    renderer_draw_frame(app);

    font_draw_str(app, app->playWidth / 2 - 6 * FONT_CHAR_W, 2, "SAVED GAMES", COL_MENU_SEL);

    SaveSlot saves[MAX_SAVES];
    int count = saves_get_list(app, saves, MAX_SAVES);

    if (count == 0) {
        font_draw_str(app, app->playWidth / 2 - 6 * FONT_CHAR_W, app->playHeight / 2, "NO SAVES FOUND", COL_UI_TEXT);
    } else {
        font_draw_str(app, 4, 4 + FONT_CHAR_H + 1, "SEL  DATE       TIME     MODE     SCORE", COL_UI_TEXT);

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
            int y = 6 + i * (FONT_CHAR_H + 1);
            font_draw_str(app, 4, y, buffer, color);
        }
    }

    font_draw_str(app, app->playWidth / 2 - 12 * FONT_CHAR_W, app->playHeight - 3 - (FONT_CHAR_H + 1), "ENTER: LOAD  DEL: DELETE  ESC: BACK", COL_UI_TEXT);
}

extern char g_nameInput[MAX_NAME_LEN];
extern int g_nameInputLen;

void renderer_draw_change_name(AppState* app) {
    renderer_clear(app, COL_MENU_BG);
    renderer_draw_frame(app);

    font_draw_str(app, app->playWidth / 2 - 5 * FONT_CHAR_W, app->playHeight / 2 - 3 * (FONT_CHAR_H + 1), "CHANGE NAME", COL_MENU_SEL);
    font_draw_str(app, app->playWidth / 2 - 10 * FONT_CHAR_W, app->playHeight / 2 - (FONT_CHAR_H + 1), "CURRENT: ", COL_UI_TEXT);
    font_draw_str(app, app->playWidth / 2 + 1 * FONT_CHAR_W, app->playHeight / 2 - (FONT_CHAR_H + 1), app->playerName, COL_MENU_SEL);
    font_draw_str(app, app->playWidth / 2 - 10 * FONT_CHAR_W, app->playHeight / 2 + (FONT_CHAR_H + 1), "NEW: ", COL_UI_TEXT);
    font_draw_str(app, app->playWidth / 2 + 1 * FONT_CHAR_W, app->playHeight / 2 + (FONT_CHAR_H + 1), g_nameInput, COL_UI_TEXT);
    // Показываем курсор
    int cursorX = app->playWidth / 2 + 1 * FONT_CHAR_W + g_nameInputLen * (FONT_CHAR_W + 1);
    int cursorY = app->playHeight / 2 + (FONT_CHAR_H + 1);
    font_draw_char(app, cursorX, cursorY, '_', COL_MENU_SEL);
    font_draw_str(app, app->playWidth / 2 - 16 * FONT_CHAR_W, app->playHeight / 2 + 3 * (FONT_CHAR_H + 1), "TYPE NAME AND PRESS ENTER, ESC TO CANCEL", COL_UI_TEXT);
}

void renderer_draw_save_preview(AppState* app) {
    renderer_clear(app, COL_GAME_BG);
    renderer_draw_frame(app);
    renderer_draw_player(app);
    renderer_draw_enemies(app);
    renderer_draw_bullets(app);
    renderer_draw_powerups(app);
    renderer_draw_ui(app);

    font_draw_str(app, app->playWidth / 2 - 12 * FONT_CHAR_W, app->playHeight / 2, "PRESS ANY KEY TO RESUME...", COL_MENU_SEL);
}

void renderer_present(AppState* app) {
    HANDLE hBuf = app->hBuffer[app->activeBuffer];

    COORD bufSize = { static_cast<SHORT>(g_backBufferWidth), static_cast<SHORT>(g_backBufferHeight) };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, static_cast<SHORT>(g_backBufferWidth - 1), static_cast<SHORT>(g_backBufferHeight - 1) };

    WriteConsoleOutputA(hBuf, g_backBuffer, bufSize, bufCoord, &writeRegion);

    // Переключаем активный буфер
    SetConsoleActiveScreenBuffer(hBuf);
    app->activeBuffer = 1 - app->activeBuffer;
}