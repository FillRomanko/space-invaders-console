#ifndef SPACE_INVADERS_SAVE_SYSTEM_H
#define SPACE_INVADERS_SAVE_SYSTEM_H

#include "types.h"
#include <windows.h>

// Инициализация системы сохранений
bool saves_init(AppState* app);

// Автосохранение (вызывается каждую секунду)
void saves_autosave(AppState* app);

// Сохранение в слот (ручное)
bool saves_save_slot(AppState* app, int slot);

// Загрузка из слота
bool saves_load_slot(AppState* app, int slot);

// Получение списка сохранений для текущего игрока
int saves_get_list(AppState* app, SaveSlot* out_saves, int max_saves);

// Удаление сохранения
bool saves_delete_slot(AppState* app, int slot);

// Шифрование/дешифрование данных (простой XOR с ключом)
void saves_encrypt(void* data, size_t size, const char* key, size_t key_len);
void saves_decrypt(void* data, size_t size, const char* key, size_t key_len);

// Вычисление контрольной суммы
uint32_t saves_checksum(const void* data, size_t size);

// Получение текущего времени как FILETIME
uint64_t saves_get_timestamp();

// Ключи шифрования (определены в save_system.cpp)
extern const char SAVE_KEY[];
extern const char LEADERBOARD_KEY[];
constexpr size_t SAVE_KEY_LEN = 31;        // strlen("SPACE_INVADERS_SAVE_KEY_2024")
constexpr size_t LEADERBOARD_KEY_LEN = 29; // strlen("SPACE_INVADERS_LB_KEY_2024")

#endif // SPACE_INVADERS_SAVE_SYSTEM_H