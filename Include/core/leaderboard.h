#ifndef SPACE_INVADERS_LEADERBOARD_H
#define SPACE_INVADERS_LEADERBOARD_H

#include "types.h"

// Инициализация таблицы рекордов
bool leaderboard_init(AppState* app);

// Добавление рекорда (если он в топ-10)
bool leaderboard_add_score(AppState* app, GameMode mode, const char* name, uint32_t score);

// Получение таблицы рекордов для режима
int leaderboard_get(AppState* app, GameMode mode, LeaderboardEntry* out_entries, int max_entries);

// Очистка таблицы (для отладки)
void leaderboard_clear(AppState* app, GameMode mode);

#endif // SPACE_INVADERS_LEADERBOARD_H