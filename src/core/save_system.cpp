#include "../../Include/core/save_system.h"
#include "../../Include/core/types.h"
#include <windows.h>
#include <cstring>
#include <cstdio>

// Ключ шифрования (простой XOR ключ на основе имени игрока + соль)
const char SAVE_KEY[] = "SPACE_INVADERS_SAVE_KEY_2024";
const char LEADERBOARD_KEY[] = "SPACE_INVADERS_LB_KEY_2024";

void saves_encrypt(void* data, size_t size, const char* key, size_t key_len) {
    uint8_t* bytes = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        bytes[i] ^= key[i % key_len] ^ static_cast<uint8_t>(i * 7 + 13);
    }
}

void saves_decrypt(void* data, size_t size, const char* key, size_t key_len) {
    // XOR дешифрование такое же как шифрование
    saves_encrypt(data, size, key, key_len);
}

uint32_t saves_checksum(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t sum = 0x12345678;
    for (size_t i = 0; i < size; ++i) {
        sum = (sum * 31) ^ bytes[i];
    }
    return sum;
}

uint64_t saves_get_timestamp() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | static_cast<uint64_t>(ft.dwLowDateTime);
}

bool saves_init(AppState* /*app*/) {
    // Файлы создаются автоматически при первом сохранении
    return true;
}

void saves_autosave(AppState* app) {
    // Сохраняем в слот 0 (автосохранение)
    saves_save_slot(app, 0);
}

bool saves_save_slot(AppState* app, int slot) {
    if (slot < 0 || slot >= MAX_SAVES) return false;

    GameState* gs = &app->game;

    // Подготавливаем слот сохранения
    SaveSlot save;
    std::memset(&save, 0, sizeof(SaveSlot));
    save.magic = SAVE_MAGIC;
    save.version = 1;
    save.timestamp = saves_get_timestamp();
    strncpy_s(save.player_name, app->playerName, MAX_NAME_LEN);
    save.mode = gs->mode;
    save.player = gs->player;
    save.enemy_spawn_timer = gs->enemy_spawn_timer;
    save.enemy_spawn_interval = gs->enemy_spawn_interval;
    save.game_time_ticks = gs->game_time_ticks;
    save.base_hp = gs->base_hp;
    save.base_max_hp = gs->base_max_hp;
    save.enemy_count = gs->enemy_count;

    // Копируем врагов
    for (int i = 0; i < gs->enemy_count && i < MAX_ENEMIES; ++i) {
        save.enemies[i] = gs->enemies[i];
    }

    // Копируем пули (все три типа)
    int up_idx = 0, ur_idx = 0, ul_idx = 0;
    for (int i = 0; i < 48 && up_idx < 48; ++i) {
        if (gs->bullets_up[i].active) {
            save.bullets_up[up_idx++] = gs->bullets_up[i];
        }
    }
    save.bullet_up_count = static_cast<uint8_t>(up_idx);

    for (int i = 0; i < 32 && ur_idx < 32; ++i) {
        if (gs->bullets_ur[i].active) {
            save.bullets_ur[ur_idx++] = gs->bullets_ur[i];
        }
    }
    save.bullet_ur_count = static_cast<uint8_t>(ur_idx);

    for (int i = 0; i < 32 && ul_idx < 32; ++i) {
        if (gs->bullets_ul[i].active) {
            save.bullets_ul[ul_idx++] = gs->bullets_ul[i];
        }
    }
    save.bullet_ul_count = static_cast<uint8_t>(ul_idx);

    // Копируем усиления
    save.powerup_count = gs->powerup_count;
    for (int i = 0; i < gs->powerup_count && i < MAX_POWERUPS; ++i) {
        save.powerups[i] = gs->powerups[i];
    }

    // Вычисляем чексумму (до шифрования)
    size_t dataSize = offsetof(SaveSlot, checksum);
    save.checksum = saves_checksum(&save, dataSize);

    // Шифруем
    saves_encrypt(&save, sizeof(SaveSlot), SAVE_KEY, sizeof(SAVE_KEY) - 1);

    // Записываем в файл (дописываем в конец или перезаписываем слот)
    // Для простоты: каждый слот - отдельный файл
    char filename[MAX_PATH_LEN];
    snprintf(filename, MAX_PATH_LEN, "save_%d.dat", slot);

    HANDLE hFile = CreateFileA(filename,
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written;
    BOOL result = WriteFile(hFile, &save, sizeof(SaveSlot), &written, NULL);
    CloseHandle(hFile);

    return result && written == sizeof(SaveSlot);
}

bool saves_load_slot(AppState* app, int slot) {
    if (slot < 0 || slot >= MAX_SAVES) return false;

    char filename[MAX_PATH_LEN];
    snprintf(filename, MAX_PATH_LEN, "save_%d.dat", slot);

    HANDLE hFile = CreateFileA(filename,
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return false;

    SaveSlot save;
    DWORD read;
    BOOL result = ReadFile(hFile, &save, sizeof(SaveSlot), &read, NULL);
    CloseHandle(hFile);

    if (!result || read != sizeof(SaveSlot)) return false;

    // Расшифровываем
    saves_decrypt(&save, sizeof(SaveSlot), SAVE_KEY, sizeof(SAVE_KEY) - 1);

    // Проверяем магию
    if (save.magic != SAVE_MAGIC) return false;

    // Проверяем чексумму
    size_t dataSize = offsetof(SaveSlot, checksum);
    uint32_t calcChecksum = saves_checksum(&save, dataSize);
    if (calcChecksum != save.checksum) return false;

    // Проверяем имя игрока
    if (strcmp(save.player_name, app->playerName) != 0) return false;

    // Восстанавливаем состояние игры
    GameState* gs = &app->game;
    gs->mode = save.mode;
    gs->player = save.player;
    gs->enemy_spawn_timer = save.enemy_spawn_timer;
    gs->enemy_spawn_interval = save.enemy_spawn_interval;
    gs->game_time_ticks = save.game_time_ticks;
    gs->base_hp = save.base_hp;
    gs->base_max_hp = save.base_max_hp;
    gs->enemy_count = save.enemy_count;

    for (int i = 0; i < save.enemy_count && i < MAX_ENEMIES; ++i) {
        gs->enemies[i] = save.enemies[i];
    }

    // Восстанавливаем пули
    memset(gs->bullets_up, 0, sizeof(gs->bullets_up));
    memset(gs->bullets_ur, 0, sizeof(gs->bullets_ur));
    memset(gs->bullets_ul, 0, sizeof(gs->bullets_ul));

    for (int i = 0; i < save.bullet_up_count && i < 48; ++i) {
        gs->bullets_up[i] = save.bullets_up[i];
    }
    gs->bullet_up_count = save.bullet_up_count;

    for (int i = 0; i < save.bullet_ur_count && i < 32; ++i) {
        gs->bullets_ur[i] = save.bullets_ur[i];
    }
    gs->bullet_ur_count = save.bullet_ur_count;

    for (int i = 0; i < save.bullet_ul_count && i < 32; ++i) {
        gs->bullets_ul[i] = save.bullets_ul[i];
    }
    gs->bullet_ul_count = save.bullet_ul_count;

    // Восстанавливаем усиления
    gs->powerup_count = save.powerup_count;
    for (int i = 0; i < save.powerup_count && i < MAX_POWERUPS; ++i) {
        gs->powerups[i] = save.powerups[i];
    }

    // Устанавливаем флаг превью
    gs->show_save_preview = 1;
    gs->paused = 0;
    gs->game_over = 0;
    gs->in_menu = 0;

    app->menuState = MENU_PLAY_NORMAL; // Будет переключено в game_start
    app->game = *gs;

    return true;
}

int saves_get_list(AppState* app, SaveSlot* out_saves, int max_saves) {
    int count = 0;

    for (int slot = 0; slot < MAX_SAVES && count < max_saves; ++slot) {
        char filename[MAX_PATH_LEN];
        snprintf(filename, MAX_PATH_LEN, "save_%d.dat", slot);

        HANDLE hFile = CreateFileA(filename,
            GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE) continue;

        SaveSlot save;
        DWORD read;
        BOOL result = ReadFile(hFile, &save, sizeof(SaveSlot), &read, NULL);
        CloseHandle(hFile);

        if (!result || read != sizeof(SaveSlot)) continue;

        // Расшифровываем для проверки
        saves_decrypt(&save, sizeof(SaveSlot), SAVE_KEY, sizeof(SAVE_KEY) - 1);

        if (save.magic != SAVE_MAGIC) continue;

        // Проверяем чексумму
        size_t dataSize = offsetof(SaveSlot, checksum);
        if (saves_checksum(&save, dataSize) != save.checksum) continue;

        // Проверяем имя игрока
        if (strcmp(save.player_name, app->playerName) != 0) continue;

        // Добавляем в список
        out_saves[count++] = save;
    }

    // Сортируем по времени (новые сверху) - пузырьковая сортировка для простоты
    for (int i = 0; i < count - 1; ++i) {
        for (int j = 0; j < count - i - 1; ++j) {
            if (out_saves[j].timestamp < out_saves[j + 1].timestamp) {
                SaveSlot tmp = out_saves[j];
                out_saves[j] = out_saves[j + 1];
                out_saves[j + 1] = tmp;
            }
        }
    }

    return count;
}

bool saves_delete_slot(AppState* /*app*/, int slot) {
    if (slot < 0 || slot >= MAX_SAVES) return false;

    char filename[MAX_PATH_LEN];
    snprintf(filename, MAX_PATH_LEN, "save_%d.dat", slot);

    return DeleteFileA(filename);
}