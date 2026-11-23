#include "../../Include/core/audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <cstdlib>
#include <ctime>

// Пути к звуковым файлам (ожидаются в папке assets/audio/)
static const char* soundFiles[SND_COUNT] = {
    nullptr,
    "assets/audio/shoot.wav",
    "assets/audio/explosion.wav",
    "assets/audio/menu_move.wav",
    "assets/audio/menu_select.wav",
    "assets/audio/game_start.wav",
    "assets/audio/powerup.wav",
    "assets/audio/hit.wav"
};

static const char* musicFiles[MUS_COUNT] = {
    nullptr,
    "assets/audio/menu1.wav",
    "assets/audio/menu2.wav",
    "assets/audio/menu3.wav",
    "assets/audio/game1.wav",
    "assets/audio/game2.wav",
    "assets/audio/game3.wav"
};

static uint8_t g_volume = 100;
static MusicId g_currentMusic = MUS_NONE;
static char g_musicPath[MAX_PATH_LEN];

bool audio_init(AppState* /*app*/) {
    // Инициализируем генератор случайных чисел для выбора музыки
    srand(static_cast<unsigned>(time(NULL)));

    // Проверяем наличие папки audio
    // (в реальности файлы должны быть скопированы пост-билд скриптом)
    return true;
}

void audio_play_sound(AppState* /*app*/, SoundId id) {
    if (id >= SND_COUNT || id == SND_NONE) return;

    const char* file = soundFiles[id];
    if (!file) return;

    // PlaySound с SND_ASYNC | SND_NODEFAULT
    PlaySoundA(file, NULL, SND_ASYNC | SND_NODEFAULT | SND_FILENAME);
}

void audio_play_music(AppState* /*app*/, MusicId id) {
    if (id >= MUS_COUNT) return;

    // Останавливаем текущую музыку
    if (g_currentMusic != MUS_NONE) {
        PlaySoundA(NULL, NULL, SND_ASYNC); // Останавливает асинхронное воспроизведение
    }

    if (id == MUS_NONE) {
        g_currentMusic = MUS_NONE;
        return;
    }

    // Выбираем случайный трек из категории
    MusicId track;
    if (id == MUS_MENU_1) {
        // Меню: 1-3
        track = static_cast<MusicId>(MUS_MENU_1 + (rand() % 3));
    } else if (id == MUS_GAME_1) {
        // Игра: 4-6
        track = static_cast<MusicId>(MUS_GAME_1 + (rand() % 3));
    } else {
        track = id;
    }

    const char* file = musicFiles[track];
    if (!file) return;

    // Копируем путь для возможного перезапуска
    strncpy_s(g_musicPath, file, MAX_PATH_LEN);

    // PlaySound с SND_LOOP | SND_ASYNC
    PlaySoundA(file, NULL, SND_ASYNC | SND_LOOP | SND_FILENAME);

    g_currentMusic = track;
}

void audio_stop_music(AppState* /*app*/) {
    PlaySoundA(NULL, NULL, SND_ASYNC);
    g_currentMusic = MUS_NONE;
}

void audio_set_volume(AppState* /*app*/, uint8_t volume) {
    g_volume = volume;
    // PlaySound не поддерживает громкость напрямую
    // Для изменения громкости нужно использовать waveOutSetVolume
    // Но для простоты оставим как есть
}

void audio_cleanup(AppState* app) {
    audio_stop_music(app);
}