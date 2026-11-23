#ifndef SPACE_INVADERS_MENU_H
#define SPACE_INVADERS_MENU_H

#include "types.h"
#include "input.h"

// Пункты главного меню
enum MainMenuItem : uint8_t {
    MAIN_PLAY_NORMAL   = 0,
    MAIN_PLAY_DEFENSE  = 1,
    MAIN_HIGHSCORES    = 2,
    MAIN_SAVES         = 3,
    MAIN_CHANGE_NAME   = 4,
    MAIN_EXIT          = 5,
    MAIN_COUNT         = 6
};

// Пункты меню паузы
enum PauseMenuItem : uint8_t {
    PAUSE_RESUME       = 0,
    PAUSE_MAIN_MENU    = 1,
    PAUSE_EXIT_GAME    = 2,
    PAUSE_COUNT        = 3
};

// Пункты меню Game Over
enum GameOverMenuItem : uint8_t {
    GO_RESTART         = 0,
    GO_MAIN_MENU       = 1,
    GO_EXIT            = 2,
    GO_COUNT           = 3
};

// Отрисовка главного меню
void menu_draw_main(AppState* app);

// Отрисовка меню паузы
void menu_draw_pause(AppState* app);

// Отрисовка меню Game Over
void menu_draw_game_over(AppState* app);

// Отрисовка таблицы рекордов
void menu_draw_highscores(AppState* app);

// Отрисовка списка сохранений
void menu_draw_saves(AppState* app);

// Отрисовка смены имени
void menu_draw_change_name(AppState* app);

// Обработка ввода в главном меню
void menu_handle_main(AppState* app, const InputState* input);

// Обработка ввода в меню паузы
void menu_handle_pause(AppState* app, const InputState* input);

// Обработка ввода в меню Game Over
void menu_handle_game_over(AppState* app, const InputState* input);

// Обработка ввода в таблице рекордов
void menu_handle_highscores(AppState* app, const InputState* input);

// Обработка ввода в сохранениях
void menu_handle_saves(AppState* app, const InputState* input);

// Обработка ввода в смене имени
void menu_handle_change_name(AppState* app, const InputState* input);

// Общая обработка ввода меню (диспетчер)
void menu_handle_input(AppState* app, const InputState* input);

// Обновление меню
void menu_update(AppState* app);

// Буфер для ввода имени (используется в меню смены имени и рендерере)
extern char g_nameInput[MAX_NAME_LEN];
extern int g_nameInputLen;

#endif // SPACE_INVADERS_MENU_H