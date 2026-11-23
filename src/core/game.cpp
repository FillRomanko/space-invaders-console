#include "../../Include/core/game.h"
#include "../../Include/core/menu.h"
#include "../../Include/core/types.h"
#include "../../Include/core/sprites.h"
#include "../../Include/core/audio.h"
#include "../../Include/core/save_system.h"
#include "../../Include/core/leaderboard.h"
#include "../../Include/core/renderer.h"
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Константы игры
static const int PLAYER_SPEED = 3;           // клеток за кадр (было 2, увеличено в 1.5x)
static const int BULLET_SPEED = 8;           // клеток за кадр (быстрее игрока в 4 раза)
static const int ENEMY_BASE_SPEED_NUM = 1;   // базовая скорость врага (числитель)
static const int ENEMY_BASE_SPEED_DEN = 4;   // знаменатель (1/4 клетки за кадр)
static const int ENEMY_SPAWN_BASE_INTERVAL = 120; // базовый интервал спавна (2 сек при 60 FPS)
static const int ENEMY_SPAWN_MIN_INTERVAL = 30;   // минимум (0.5 сек)
static const int POWERUP_SPAWN_CHANCE = 15;       // % шанс выпадения усиления
// POWERUP_SPEED теперь использует такую же систему как враги (ENEMY_BASE_SPEED_NUM/ENEMY_BASE_SPEED_DEN)
static const int SHIELD_DURATION = 10 * 60;       // 10 секунд
static const int TRIPLE_DURATION = 8 * 60;        // 8 секунд
static const int TRIPLE_EXTEND = 5 * 60;          // продление при повторном подборе

// Генератор случайных чисел
static uint32_t g_rng_state = 0;

static uint32_t rng_next() {
    g_rng_state = g_rng_state * 1664525 + 1013904223;
    return g_rng_state;
}

static int rng_range(int min, int max) {
    return min + (rng_next() % (max - min + 1));
}

void game_init_new(AppState* app, GameMode mode) {
    GameState* gs = &app->game;
    memset(gs, 0, sizeof(GameState));

    gs->mode = mode;
    gs->play_width = app->playWidth;
    gs->play_height = app->playHeight;
    gs->offset_x = app->offsetX;
    gs->offset_y = app->offsetY;

    // Игрок в центре снизу
    gs->player.x = static_cast<int16_t>(gs->play_width / 2);
    gs->player.y = static_cast<int16_t>(gs->play_height - 5);
    gs->player.hp = 3;
    gs->player.max_hp = 3;
    gs->player.score = 0;
    strncpy_s(gs->player_name, app->playerName, MAX_NAME_LEN);

    // Враги
    gs->enemy_spawn_interval = ENEMY_SPAWN_BASE_INTERVAL;
    gs->enemy_spawn_timer = 0;
    gs->enemy_speed_num = ENEMY_BASE_SPEED_NUM;
    gs->enemy_speed_den = ENEMY_BASE_SPEED_DEN;
    gs->game_time_ticks = 0;
    gs->enemy_count = 0;

    // База для MODE_DEFENSE
    if (mode == MODE_DEFENSE) {
        gs->base_hp = 10;
        gs->base_max_hp = 10;
    }

    // Пули
    gs->bullet_up_count = 0;
    gs->bullet_ur_count = 0;
    gs->bullet_ul_count = 0;

    // Усиления
    gs->powerup_count = 0;

    // Флаги
    gs->paused = 0;
    gs->game_over = 0;
    gs->in_menu = 0;
    gs->show_save_preview = 0;

    app->autosaveTimer = 60; // 1 секунда
    app->menuState = (mode == MODE_NORMAL) ? MENU_PLAY_NORMAL : MENU_PLAY_DEFENSE;

    // Инициализируем RNG
    g_rng_state = static_cast<uint32_t>(time(NULL)) ^ static_cast<uint32_t>(GetTickCount64());

    // Звук начала игры
    audio_play_sound(app, SND_GAME_START);
    audio_play_music(app, MUS_GAME_1);
}

void game_init_from_save(AppState* app, const SaveSlot* /*save*/) {
    // Состояние уже восстановлено в saves_load_slot
    // Здесь только доп. инициализация
    app->autosaveTimer = 60;
    audio_play_music(app, MUS_GAME_1);
}

void game_tick(AppState* app, const InputState* input, float /*dt*/) {
    GameState* gs = &app->game;

    if (gs->paused || gs->game_over || gs->in_menu || gs->show_save_preview) {
        return;
    }

    gs->game_time_ticks++;
    app->autosaveTimer--;
    if (app->autosaveTimer <= 0) {
        saves_autosave(app);
        app->autosaveTimer = 60;
    }

    // Обновление таймеров баффов
    if (gs->player.shield_active) {
        // Щит не имеет таймера, он отключается при попадании
    }
    if (gs->player.triple_straight_timer > 0) {
        gs->player.triple_straight_timer--;
    }
    if (gs->player.triple_diagonal_timer > 0) {
        gs->player.triple_diagonal_timer--;
    }

    // Очки за выживание (1 очко в секунду)
    if (gs->game_time_ticks % 60 == 0) {
        gs->player.score += 1;
    }

    // ===== УПРАВЛЕНИЕ ИГРОКА =====
    int dx = 0, dy = 0;

    // Клавиатура - проверка удерживаемых клавиш (input_is_held)
    if (input_is_held(input, KEY_UP) || input_is_held(input, KEY_W)) dy -= PLAYER_SPEED;
    if (input_is_held(input, KEY_DOWN) || input_is_held(input, KEY_S)) dy += PLAYER_SPEED;
    if (input_is_held(input, KEY_LEFT) || input_is_held(input, KEY_A)) dx -= PLAYER_SPEED;
    if (input_is_held(input, KEY_RIGHT) || input_is_held(input, KEY_D)) dx += PLAYER_SPEED;

    // Мышь (если двинулась)
    if (input->mouse_moved) {
        int targetX = input->mouse_x;
        int targetY = input->mouse_y;
        dx = (targetX - gs->player.x) / 2;
        dy = (targetY - gs->player.y) / 2;
        // Ограничиваем скорость мышью
        if (dx > PLAYER_SPEED) dx = PLAYER_SPEED;
        if (dx < -PLAYER_SPEED) dx = -PLAYER_SPEED;
        if (dy > PLAYER_SPEED) dy = PLAYER_SPEED;
        if (dy < -PLAYER_SPEED) dy = -PLAYER_SPEED;
    }

    // Применяем движение с проверкой границ
    int newX = gs->player.x + dx;
    int newY = gs->player.y + dy;

    int margin = SPRITE_PLAYER_W / 2 + (gs->player.shield_active ? gs->player.shield_hitbox_extra : 0);
    if (newX - margin < 1) newX = 1 + margin;
    if (newX + margin >= gs->play_width - 1) newX = gs->play_width - 2 - margin;
    if (newY - SPRITE_PLAYER_H < 1) newY = 1 + SPRITE_PLAYER_H;
    if (newY >= gs->play_height - 1) newY = gs->play_height - 2;

    gs->player.x = static_cast<int16_t>(newX);
    gs->player.y = static_cast<int16_t>(newY);

    // Стрельба - проверка нажатия в текущем кадре
    if (input_is_pressed(input, KEY_SPACE) || input->mouse_left) {
        game_player_shoot(app);
    }

    // Пауза
    if (input_is_pressed(input, KEY_ESCAPE)) {
        game_toggle_pause(app);
        return;
    }

    // ===== ОБНОВЛЕНИЕ ПУЛЬ =====
    // Вертикальные
    for (int i = 0; i < gs->bullet_up_count; ++i) {
        Bullet* b = &gs->bullets_up[i];
        if (!b->active) continue;
        b->y -= BULLET_SPEED;
        if (b->y < 1) {
            b->active = 0;
        }
    }

    // Вверх-вправо
    for (int i = 0; i < gs->bullet_ur_count; ++i) {
        Bullet* b = &gs->bullets_ur[i];
        if (!b->active) continue;
        b->y -= BULLET_SPEED;
        b->x += BULLET_SPEED / 2;
        if (b->y < 1 || b->x >= gs->play_width - 1) {
            b->active = 0;
        }
    }

    // Вверх-влево
    for (int i = 0; i < gs->bullet_ul_count; ++i) {
        Bullet* b = &gs->bullets_ul[i];
        if (!b->active) continue;
        b->y -= BULLET_SPEED;
        b->x -= BULLET_SPEED / 2;
        if (b->y < 1 || b->x < 1) {
            b->active = 0;
        }
    }

    // ===== ОБНОВЛЕНИЕ ВРАГОВ =====
    gs->enemy_spawn_timer++;
    if (gs->enemy_spawn_timer >= gs->enemy_spawn_interval) {
        game_spawn_enemy(app);
        gs->enemy_spawn_timer = 0;
    }

    // Движение врагов вниз
    // Скорость растет вогнуто: speed = base * (1 + log(1 + time/1000))
    float speedMult = 1.0f + logf(1.0f + gs->game_time_ticks / 1000.0f) * 0.3f;
    int enemySpeedNum = static_cast<int>(ENEMY_BASE_SPEED_NUM * speedMult * 100);
    int enemySpeedDen = ENEMY_BASE_SPEED_DEN * 100;

    // Аккумулятор для дробной скорости врагов
    static int enemyAccumulator = 0;
    enemyAccumulator += enemySpeedNum;
    int moveSteps = enemyAccumulator / enemySpeedDen;
    enemyAccumulator %= enemySpeedDen;

    // Аккумулятор для дробной скорости усилений (та же скорость что и враги)
    static int powerupAccumulator = 0;
    powerupAccumulator += enemySpeedNum;
    int powerupMoveSteps = powerupAccumulator / enemySpeedDen;
    powerupAccumulator %= enemySpeedDen;

    for (int step = 0; step < moveSteps; ++step) {
        for (int i = 0; i < gs->enemy_count; ++i) {
            Enemy* e = &gs->enemies[i];
            if (!e->active) continue;

            e->y += 1;

            // Проверка столкновения с игроком - хитбокс на весь спрайт (31x15), поднят на 4 вверх
            // Спрайт рисуется: верх в y - SPRITE_PLAYER_H + 1, низ в y
            // Центр спрайта по X: player.x, по Y: player.y - SPRITE_PLAYER_H/2
            // Поднимаем хитбокс на 4 символа выше центра для лучшего ощущения
            int px = gs->player.x;
            int py = gs->player.y - SPRITE_PLAYER_H / 2 - 4;  // Центр спрайта по вертикали + 4 вверх

            // Хитбокс = половина размеров спрайта (+щит), расширен вниз на 4 для раннего срабатывания
            int pHitbox = SPRITE_PLAYER_W / 2 + (gs->player.shield_active ? gs->player.shield_hitbox_extra : 0);
            int pHitboxV = SPRITE_PLAYER_H / 2 + 4 + (gs->player.shield_active ? gs->player.shield_hitbox_extra : 0);

            if (abs(e->x - px) <= pHitbox && abs(e->y - py) <= pHitboxV) {
                if (gs->player.shield_active) {
                    // Щит поглощает удар
                    gs->player.shield_active = 0;
                    gs->player.shield_hitbox_extra = 0;
                    e->active = 0;
                    gs->player.score += 50;
                    audio_play_sound(app, SND_EXPLOSION);
                    game_update_difficulty(app, static_cast<int16_t>(e->y));
                } else {
                    // Игрок получает урон: враг исчезает, HP -1, игрок остаётся на месте (как при щите)
                    gs->player.hp--;
                    audio_play_sound(app, SND_HIT);
                    e->active = 0;
                    gs->player.score += 10; // небольшой бонус за выживание

                    if (gs->player.hp <= 0) {
                        gs->game_over = 1;
                        audio_stop_music(app);
                        leaderboard_add_score(app, gs->mode, app->playerName, gs->player.score);
                    }
                    // НЕ делаем респавн - игрок остаётся на месте
                    return;
                }
            }

            // Проверка достижения низа (для MODE_DEFENSE)
            if (gs->mode == MODE_DEFENSE && e->y >= gs->play_height - 2) {
                game_damage_base(app);
                e->active = 0;
                if (gs->base_hp <= 0) {
                    gs->game_over = 1;
                }
            }
        }
    }

    // Удаление мертвых врагов (сжатие массива)
    int writeIdx = 0;
    for (int i = 0; i < gs->enemy_count; ++i) {
        if (gs->enemies[i].active) {
            if (writeIdx != i) {
                gs->enemies[writeIdx] = gs->enemies[i];
            }
            writeIdx++;
        }
    }
    gs->enemy_count = static_cast<uint8_t>(writeIdx);

    // ===== ПРОВЕРКА ПОПАДАНИЙ ПУЛЬ ПО ВРАГАМ =====
    // Вертикальные пули
    for (int bi = 0; bi < gs->bullet_up_count; ++bi) {
        Bullet* b = &gs->bullets_up[bi];
        if (!b->active) continue;

        for (int ei = 0; ei < gs->enemy_count; ++ei) {
            Enemy* e = &gs->enemies[ei];
            if (!e->active) continue;

            // Хитбокс врага (увеличен для лучшего попадания)
            if (abs(b->x - e->x) <= SPRITE_ENEMY_W / 2 + 2 && abs(b->y - e->y) <= SPRITE_ENEMY_H / 2 + 2) {
                b->active = 0;
                e->active = 0;
                gs->player.score += 100;
                audio_play_sound(app, SND_EXPLOSION);
                game_update_difficulty(app, static_cast<int16_t>(e->y));

                // Шанс выпадения усиления
                if (rng_range(1, 100) <= POWERUP_SPAWN_CHANCE) {
                    game_spawn_powerup(app, static_cast<int16_t>(e->x), static_cast<int16_t>(e->y));
                }
                break;
            }
        }
    }

    // Диагональные пули (общий цикл для UR и UL)
    for (int bi = 0; bi < gs->bullet_ur_count; ++bi) {
        Bullet* b = &gs->bullets_ur[bi];
        if (!b->active) continue;

        for (int ei = 0; ei < gs->enemy_count; ++ei) {
            Enemy* e = &gs->enemies[ei];
            if (!e->active) continue;

            if (abs(b->x - e->x) <= SPRITE_ENEMY_W / 2 + 2 && abs(b->y - e->y) <= SPRITE_ENEMY_H / 2 + 2) {
                b->active = 0;
                e->active = 0;
                gs->player.score += 100;
                audio_play_sound(app, SND_EXPLOSION);
                game_update_difficulty(app, static_cast<int16_t>(e->y));
                if (rng_range(1, 100) <= POWERUP_SPAWN_CHANCE) {
                    game_spawn_powerup(app, static_cast<int16_t>(e->x), static_cast<int16_t>(e->y));
                }
                break;
            }
        }
    }

    for (int bi = 0; bi < gs->bullet_ul_count; ++bi) {
        Bullet* b = &gs->bullets_ul[bi];
        if (!b->active) continue;

        for (int ei = 0; ei < gs->enemy_count; ++ei) {
            Enemy* e = &gs->enemies[ei];
            if (!e->active) continue;

            if (abs(b->x - e->x) <= SPRITE_ENEMY_W / 2 + 2 && abs(b->y - e->y) <= SPRITE_ENEMY_H / 2 + 2) {
                b->active = 0;
                e->active = 0;
                gs->player.score += 100;
                audio_play_sound(app, SND_EXPLOSION);
                game_update_difficulty(app, static_cast<int16_t>(e->y));
                if (rng_range(1, 100) <= POWERUP_SPAWN_CHANCE) {
                    game_spawn_powerup(app, static_cast<int16_t>(e->x), static_cast<int16_t>(e->y));
                }
                break;
            }
        }
    }

    // ===== ОБНОВЛЕНИЕ УСИЛЕНИЙ =====
    for (int i = 0; i < gs->powerup_count; ++i) {
        PowerUp* p = &gs->powerups[i];
        if (!p->active) continue;

        // Усиления падают с той же скоростью, что и враги
        for (int step = 0; step < powerupMoveSteps; ++step) {
            p->y += 1;
        }

        // Проверка подбора игроком - хитбокс на весь спрайт игрока (с учётом щита), поднят на 4 вверх
        // Игрок: центр X = player.x, центр Y = player.y - SPRITE_PLAYER_H/2 - 4
        // Хитбокс: ±SPRITE_PLAYER_W/2 по X, ±SPRITE_PLAYER_H/2 по Y (+щит)
        int px = gs->player.x;
        int py = gs->player.y - SPRITE_PLAYER_H / 2 - 4;
        int pHitbox = SPRITE_PLAYER_W / 2 + (gs->player.shield_active ? gs->player.shield_hitbox_extra : 0);
        int pHitboxV = SPRITE_PLAYER_H / 2 + (gs->player.shield_active ? gs->player.shield_hitbox_extra : 0);

        if (abs(p->x - px) <= pHitbox && abs(p->y - py) <= pHitboxV) {
            game_collect_powerup(app, i);
            p->active = 0;
        }

        // Удаление вышедших за экран
        if (p->y >= gs->play_height - 1) {
            p->active = 0;
        }
    }

    // Сжатие массива усилений
    writeIdx = 0;
    for (int i = 0; i < gs->powerup_count; ++i) {
        if (gs->powerups[i].active) {
            if (writeIdx != i) gs->powerups[writeIdx] = gs->powerups[i];
            writeIdx++;
        }
    }
    gs->powerup_count = static_cast<uint8_t>(writeIdx);

    // ===== ПРОВЕРКА УСЛОВИЙ ПОБЕДЫ/ПОРАЖЕНИЯ =====
    game_check_win_lose(app);
}

// Вспомогательная функция: найти неактивный слот для пули или добавить новый
static Bullet* get_free_bullet(Bullet* bullets, uint8_t* count, uint8_t max_count) {
    // Сначала ищем неактивную пулю для переиспользования
    for (uint8_t i = 0; i < *count; ++i) {
        if (!bullets[i].active) {
            return &bullets[i];
        }
    }
    // Если все активны и есть место - добавляем новую
    if (*count < max_count) {
        return &bullets[(*count)++];
    }
    // Пула переполнен - перезаписываем самую старую (индекс 0) и сдвигаем
    // Для простоты просто возвращаем nullptr, пуля не создастся
    return nullptr;
}

void game_player_shoot(AppState* app) {
    GameState* gs = &app->game;
    Player* pl = &gs->player;

    // Вертикальная пуля (всегда)
    Bullet* b = get_free_bullet(gs->bullets_up, &gs->bullet_up_count, 48);
    if (b) {
        b->x = pl->x;
        b->y = static_cast<int16_t>(pl->y - SPRITE_PLAYER_H / 2 - 1);
        b->active = 1;
    }

    // Тройные прямые
    if (pl->triple_straight_timer > 0) {
        b = get_free_bullet(gs->bullets_up, &gs->bullet_up_count, 48);
        if (b) {
            b->x = static_cast<int16_t>(pl->x - 3);
            b->y = static_cast<int16_t>(pl->y - SPRITE_PLAYER_H / 2 - 1);
            b->active = 1;
        }
        b = get_free_bullet(gs->bullets_up, &gs->bullet_up_count, 48);
        if (b) {
            b->x = static_cast<int16_t>(pl->x + 3);
            b->y = static_cast<int16_t>(pl->y - SPRITE_PLAYER_H / 2 - 1);
            b->active = 1;
        }
    }

    // Тройные диагональные
    if (pl->triple_diagonal_timer > 0) {
        // Вверх-вправо
        b = get_free_bullet(gs->bullets_ur, &gs->bullet_ur_count, 32);
        if (b) {
            b->x = static_cast<int16_t>(pl->x + 2);
            b->y = static_cast<int16_t>(pl->y - SPRITE_PLAYER_H / 2 - 1);
            b->active = 1;
        }
        // Вверх-влево
        b = get_free_bullet(gs->bullets_ul, &gs->bullet_ul_count, 32);
        if (b) {
            b->x = static_cast<int16_t>(pl->x - 2);
            b->y = static_cast<int16_t>(pl->y - SPRITE_PLAYER_H / 2 - 1);
            b->active = 1;
        }

        // Если есть оба баффа - две дополнительные диагональные чуть ниже
        if (pl->triple_straight_timer > 0) {
            b = get_free_bullet(gs->bullets_ur, &gs->bullet_ur_count, 32);
            if (b) {
                b->x = static_cast<int16_t>(pl->x + 4);
                b->y = static_cast<int16_t>(pl->y - SPRITE_PLAYER_H / 2);
                b->active = 1;
            }
            b = get_free_bullet(gs->bullets_ul, &gs->bullet_ul_count, 32);
            if (b) {
                b->x = static_cast<int16_t>(pl->x - 4);
                b->y = static_cast<int16_t>(pl->y - SPRITE_PLAYER_H / 2);
                b->active = 1;
            }
        }
    }

    audio_play_sound(app, SND_SHOOT);
}

void game_spawn_enemy(AppState* app) {
    GameState* gs = &app->game;

    if (gs->enemy_count >= MAX_ENEMIES) return;

    Enemy* e = &gs->enemies[gs->enemy_count++];
    e->x = static_cast<int16_t>(rng_range(SPRITE_ENEMY_W / 2 + 2, gs->play_width - SPRITE_ENEMY_W / 2 - 2));
    e->y = -1; // спавн чуть выше верхней линии рамки (вылезает из неё)
    e->active = 1;
    e->frame = 0;
}

void game_spawn_powerup(AppState* app, int16_t x, int16_t y) {
    GameState* gs = &app->game;

    if (gs->powerup_count >= MAX_POWERUPS) return;

    PowerUp* p = &gs->powerups[gs->powerup_count++];
    p->x = x;
    p->y = y;
    p->type = static_cast<PowerUpType>(rng_range(1, 3)); // 1=shield, 2=triple straight, 3=triple diagonal
    p->active = 1;
}

void game_collect_powerup(AppState* app, int powerup_index) {
    GameState* gs = &app->game;
    PowerUp* p = &gs->powerups[powerup_index];
    Player* pl = &gs->player;

    audio_play_sound(app, SND_POWERUP);

    switch (p->type) {
        case POWERUP_SHIELD:
            if (!pl->shield_active) {
                pl->shield_active = 1;
                pl->shield_hitbox_extra = 2; // увеличивает хитбокс
            }
            // Не стакается
            break;

        case POWERUP_TRIPLE_STRAIGHT:
            if (pl->triple_straight_timer > 0) {
                // Уже есть - продлеваем с бонусом
                pl->triple_straight_timer += TRIPLE_EXTEND;
            } else {
                pl->triple_straight_timer = TRIPLE_DURATION;
            }
            break;

        case POWERUP_TRIPLE_DIAGONAL:
            if (pl->triple_diagonal_timer > 0) {
                pl->triple_diagonal_timer += TRIPLE_EXTEND;
            } else {
                pl->triple_diagonal_timer = TRIPLE_DURATION;
            }
            break;
    }
}

void game_update_difficulty(AppState* app, int16_t killed_enemy_y) {
    GameState* gs = &app->game;

    // Цель: враги умирают по центру по высоте
    int targetY = gs->play_height / 2;
    int diff = killed_enemy_y - targetY;

    // Если враг умер выше центра (diff < 0) - игрок успевает, увеличиваем спавн
    // Если враг умер ниже центра (diff > 0) - игрок не успевает, уменьшаем спавн
    if (diff < -5) {
        // Умер далеко выше - слишком легко
        if (gs->enemy_spawn_interval > ENEMY_SPAWN_MIN_INTERVAL) {
            gs->enemy_spawn_interval -= 2;
        }
    } else if (diff > 5) {
        // Умер ниже - слишком сложно
        if (gs->enemy_spawn_interval < ENEMY_SPAWN_BASE_INTERVAL * 2) {
            gs->enemy_spawn_interval += 3;
        }
    }

    // Ограничиваем
    if (gs->enemy_spawn_interval < ENEMY_SPAWN_MIN_INTERVAL) {
        gs->enemy_spawn_interval = ENEMY_SPAWN_MIN_INTERVAL;
    }
    if (gs->enemy_spawn_interval > ENEMY_SPAWN_BASE_INTERVAL * 3) {
        gs->enemy_spawn_interval = ENEMY_SPAWN_BASE_INTERVAL * 3;
    }
}

void game_damage_base(AppState* app) {
    GameState* gs = &app->game;

    if (gs->base_hp > 0) {
        gs->base_hp--;
        audio_play_sound(app, SND_HIT);
    }
}

void game_player_death(AppState* app) {
    GameState* gs = &app->game;

    gs->player.hp--;
    audio_play_sound(app, SND_HIT);

    if (gs->player.hp <= 0) {
        gs->game_over = 1;
        audio_stop_music(app);
        // Записываем рекорд
        leaderboard_add_score(app, gs->mode, app->playerName, gs->player.score);
    } else {
        // Респавн игрока в центре
        gs->player.x = static_cast<int16_t>(gs->play_width / 2);
        gs->player.y = static_cast<int16_t>(gs->play_height - 5);
        gs->player.shield_active = 0;
        gs->player.shield_hitbox_extra = 0;
    }
}

void game_check_win_lose(AppState* app) {
    GameState* gs = &app->game;

    if (gs->mode == MODE_DEFENSE && gs->base_hp <= 0) {
        gs->game_over = 1;
        audio_stop_music(app);
        leaderboard_add_score(app, gs->mode, app->playerName, gs->player.score);
    }
}

void game_toggle_pause(AppState* app) {
    GameState* gs = &app->game;

    if (gs->paused) {
        // Resume -> мелкий шрифт (игра)
        renderer_set_font_mode(app, false);
        gs->paused = 0;
        gs->in_menu = 0;
        audio_play_music(app, MUS_GAME_1);
    } else {
        // Pause -> мелкий шрифт (пауза использует игровой шрифт)
        renderer_set_font_mode(app, false);
        gs->paused = 1;
        gs->in_menu = 1;
        app->menuState = MENU_PAUSE;
        app->menuSelection = 0;
        app->menuItemCount = PAUSE_COUNT;
        audio_stop_music(app);
        // Автосохранение при паузе
        saves_autosave(app);
    }
}

void game_exit_to_menu(AppState* app) {
    GameState* gs = &app->game;

    saves_autosave(app);
    audio_stop_music(app);
    audio_play_music(app, MUS_MENU_1);

    gs->paused = 0;
    gs->game_over = 0;
    gs->in_menu = 1;
    gs->show_save_preview = 0;
    app->menuState = MENU_MAIN;
    app->menuSelection = 0;
    app->menuItemCount = MAIN_COUNT;

    // Главное меню -> крупный шрифт
    renderer_set_font_mode(app, true);
}

void game_start(AppState* app, GameMode mode) {
    game_init_new(app, mode);
    // Игра -> мелкий шрифт
    renderer_set_font_mode(app, false);
}