#ifndef SPACE_INVADERS_GAME_H
#define SPACE_INVADERS_GAME_H

#include "types.h"
#include "input.h"

// Инициализация новой игры
void game_init_new(AppState* app, GameMode mode);

// Инициализация игры из сохранения
void game_init_from_save(AppState* app, const SaveSlot* save);

// Основной игровой цикл (один кадр)
void game_tick(AppState* app, const InputState* input, float dt);

// Начало игры (из меню)
void game_start(AppState* app, GameMode mode);

// Пауза/продолжение
void game_toggle_pause(AppState* app);

// Выход в меню
void game_exit_to_menu(AppState* app);

// Смерть игрока
void game_player_death(AppState* app);

// Проверка условий победы/поражения
void game_check_win_lose(AppState* app);

// Спавн врага
void game_spawn_enemy(AppState* app);

// Спавн усиления
void game_spawn_powerup(AppState* app, int16_t x, int16_t y);

// Подбор усиления игроком
void game_collect_powerup(AppState* app, int powerup_index);

// Выстрел игрока
void game_player_shoot(AppState* app);

// Обновление динамической сложности
void game_update_difficulty(AppState* app, int16_t killed_enemy_y);

// Применение урона базе (MODE_DEFENSE)
void game_damage_base(AppState* app);

#endif // SPACE_INVADERS_GAME_H