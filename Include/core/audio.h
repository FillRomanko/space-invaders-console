#ifndef SPACE_INVADERS_AUDIO_H
#define SPACE_INVADERS_AUDIO_H

#include "types.h"
#include <windows.h>

// Идентификаторы звуков
enum SoundId : uint8_t {
    SND_NONE = 0,
    SND_SHOOT,
    SND_EXPLOSION,
    SND_MENU_MOVE,
    SND_MENU_SELECT,
    SND_GAME_START,
    SND_POWERUP,
    SND_HIT,
    SND_COUNT
};

// Идентификаторы музыкальных треков
enum MusicId : uint8_t {
    MUS_NONE = 0,
    MUS_MENU_1,
    MUS_MENU_2,
    MUS_MENU_3,
    MUS_GAME_1,
    MUS_GAME_2,
    MUS_GAME_3,
    MUS_COUNT
};

// Инициализация аудио
bool audio_init(AppState* app);

// Воспроизведение звукового эффекта (асинхронно)
void audio_play_sound(AppState* app, SoundId id);

// Воспроизведение музыки (циклически, асинхронно)
void audio_play_music(AppState* app, MusicId id);

// Остановка музыки
void audio_stop_music(AppState* app);

// Установка громкости (0-100)
void audio_set_volume(AppState* app, uint8_t volume);

// Очистка
void audio_cleanup(AppState* app);

#endif // SPACE_INVADERS_AUDIO_H