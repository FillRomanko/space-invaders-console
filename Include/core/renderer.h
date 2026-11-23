#ifndef SPACE_INVADERS_RENDERER_H
#define SPACE_INVADERS_RENDERER_H

#include "types.h"

// Инициализация рендерера (создание второго буфера)
bool renderer_init(AppState* app);

// Очистка буфера заданным цветом
void renderer_clear(AppState* app, WORD color);

// Отрисовка одного символа в буфер (обычный текст)
void renderer_draw_char(AppState* app, int x, int y, char c, WORD color);

// Отрисовка строки (обычный текст)
void renderer_draw_str(AppState* app, int x, int y, const char* str, WORD color);

// Отрисовка спрайта (многострочного) - обычный
void renderer_draw_sprite(AppState* app, int x, int y, const char* sprite[], int lines, WORD color);

// Отрисовка спрайта с прозрачностью (пропуск пробелов) - обычный
void renderer_draw_sprite_transparent(AppState* app, int x, int y, const char* sprite[], int lines, WORD color, char transparent_char);

// --- НОВОЕ: ASCII Font Sprite System ---

// Загрузка шрифта из файла (96 символов 32..127, каждый 6x6)
bool font_load_from_file(AppState* app, const char* filename);

// Отрисовка одного символа через спрайт шрифта (6x6)
void font_draw_char(AppState* app, int x, int y, char c, WORD color);

// Отрисовка строки через спрайт шрифта
void font_draw_str(AppState* app, int x, int y, const char* str, WORD color);

// Отрисовка числа через спрайт шрифта
void font_draw_int(AppState* app, int x, int y, int value, WORD color);

// Отрисовка меню через спрайт шрифта
void font_draw_menu(AppState* app, int x, int y, const char* items[], int count, int selected, WORD normal_color, WORD selected_color);

// ----------------------------------------

// Отрисовка рамки игрового поля
void renderer_draw_frame(AppState* app);

// Отрисовка игрока
void renderer_draw_player(AppState* app);

// Отрисовка врагов
void renderer_draw_enemies(AppState* app);

// Отрисовка пуль
void renderer_draw_bullets(AppState* app);

// Отрисовка усилений
void renderer_draw_powerups(AppState* app);

// Отрисовка UI (очки, жизни, база) - через font sprite
void renderer_draw_ui(AppState* app);

// Отрисовка меню - через font sprite
void renderer_draw_menu(AppState* app);

// Отрисовка экрана паузы - через font sprite
void renderer_draw_pause(AppState* app);

// Отрисовка экрана Game Over - через font sprite
void renderer_draw_game_over(AppState* app);

// Отрисовка таблицы рекордов - через font sprite
void renderer_draw_leaderboard(AppState* app, GameMode mode);

// Отрисовка списка сохранений - через font sprite
void renderer_draw_saves(AppState* app);

// Отрисовка экрана смены имени - через font sprite
void renderer_draw_change_name(AppState* app);

// Отрисовка превью сохранения
void renderer_draw_save_preview(AppState* app);

// Переключение буферов (показ)
void renderer_present(AppState* app);

// Получение указателя на текущий буфер для прямого доступа
CHAR_INFO* renderer_get_buffer(AppState* app);

// Явная установка режима шрифта (вызывать ТОЛЬКО при смене состояния меню/игры).
// isLargeFont = true -> крупный шрифт для меню (MENU_MAIN, HIGHSCORES, SAVES, CHANGE_NAME, GAME_OVER)
// isLargeFont = false -> мелкий шрифт для игры и паузы
void renderer_set_font_mode(AppState* app, bool isLargeFont);

#endif // SPACE_INVADERS_RENDERER_H