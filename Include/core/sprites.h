#ifndef SPACE_INVADERS_SPRITES_H
#define SPACE_INVADERS_SPRITES_H

// Спрайт игрока (31 строка)
extern const char* SPRITE_PLAYER[31];
extern const int SPRITE_PLAYER_H;
extern const int SPRITE_PLAYER_W;

// Спрайт врага (18 строк)
extern const char* SPRITE_ENEMY[18];
extern const int SPRITE_ENEMY_H;
extern const int SPRITE_ENEMY_W;

// Спрайт зелёных деталей врага (14 строк)
extern const char* SPRITE_ENEMY_GREEN[14];
extern const int SPRITE_ENEMY_GREEN_H;
extern const int SPRITE_ENEMY_GREEN_W;

// Спрайт пули (1 символ)
extern const char SPRITE_BULLET;

// Спрайт усилений (3 типа)
extern const char* SPRITE_POWERUP_SHIELD[3];
extern const char* SPRITE_POWERUP_TRIPLE_STRAIGHT[3];
extern const char* SPRITE_POWERUP_TRIPLE_DIAGONAL[3];
extern const int SPRITE_POWERUP_H;
extern const int SPRITE_POWERUP_W;

#endif // SPACE_INVADERS_SPRITES_H