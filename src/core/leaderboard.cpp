#include "../../Include/core/leaderboard.h"
#include "../../Include/core/types.h"
#include <windows.h>
#include <cstring>

static const char* LEADERBOARD_FILE_NORMAL = "leaderboard_normal.dat";
static const char* LEADERBOARD_FILE_DEFENSE = "leaderboard_defense.dat";

// Ключ шифрования (определён в save_system.cpp)
extern const char LEADERBOARD_KEY[];
extern const char SAVE_KEY[];

// Функции из save_system.cpp
extern uint32_t saves_checksum(const void* data, size_t size);
extern void saves_encrypt(void* data, size_t size, const char* key, size_t key_len);
extern void saves_decrypt(void* data, size_t size, const char* key, size_t key_len);
extern uint64_t saves_get_timestamp();

bool leaderboard_init(AppState* /*app*/) {
    // Файлы создаются автоматически
    return true;
}

static const char* get_leaderboard_file(GameMode mode) {
    return (mode == MODE_NORMAL) ? LEADERBOARD_FILE_NORMAL : LEADERBOARD_FILE_DEFENSE;
}

bool leaderboard_add_score(AppState* /*app*/, GameMode mode, const char* name, uint32_t score) {
    const char* filename = get_leaderboard_file(mode);

    // Читаем текущую таблицу
    LeaderboardEntry entries[MAX_LEADERBOARD];
    int count = 0;

    HANDLE hFile = CreateFileA(filename,
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        LeaderboardEntry entry;
        DWORD read;
        while (count < MAX_LEADERBOARD && ReadFile(hFile, &entry, sizeof(LeaderboardEntry), &read, NULL) && read == sizeof(LeaderboardEntry)) {
            saves_decrypt(&entry, sizeof(LeaderboardEntry), LEADERBOARD_KEY, strlen(LEADERBOARD_KEY));
            if (entry.magic == LEADERBOARD_MAGIC) {
                size_t dataSize = offsetof(LeaderboardEntry, checksum);
                if (saves_checksum(&entry, dataSize) == entry.checksum) {
                    entries[count++] = entry;
                }
            }
        }
        CloseHandle(hFile);
    }

    // Проверяем, попадает ли в топ
    int insertPos = -1;
    for (int i = 0; i < count; ++i) {
        if (score > entries[i].score) {
            insertPos = i;
            break;
        }
    }

    if (insertPos == -1 && count >= MAX_LEADERBOARD) {
        return false; // Не в топе
    }

    // Создаем новую запись
    LeaderboardEntry newEntry;
    std::memset(&newEntry, 0, sizeof(newEntry));
    newEntry.magic = LEADERBOARD_MAGIC;
    strncpy_s(newEntry.name, name, MAX_NAME_LEN);
    newEntry.score = score;
    newEntry.timestamp = saves_get_timestamp();
    newEntry.mode = mode;
    size_t dataSize = offsetof(LeaderboardEntry, checksum);
    newEntry.checksum = saves_checksum(&newEntry, dataSize);

    // Сдвигаем массив
    if (count < MAX_LEADERBOARD) {
        count++;
    }
    for (int i = count - 1; i > insertPos; --i) {
        entries[i] = entries[i - 1];
    }
    if (insertPos >= 0) {
        entries[insertPos] = newEntry;
    } else {
        entries[count - 1] = newEntry;
    }

    // Записываем обратно
    hFile = CreateFileA(filename,
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return false;

    for (int i = 0; i < count; ++i) {
        LeaderboardEntry enc = entries[i];
        saves_encrypt(&enc, sizeof(LeaderboardEntry), LEADERBOARD_KEY, strlen(LEADERBOARD_KEY));
        DWORD written;
        WriteFile(hFile, &enc, sizeof(LeaderboardEntry), &written, NULL);
    }

    CloseHandle(hFile);
    return true;
}

int leaderboard_get(AppState* /*app*/, GameMode mode, LeaderboardEntry* out_entries, int max_entries) {
    const char* filename = get_leaderboard_file(mode);

    int count = 0;
    HANDLE hFile = CreateFileA(filename,
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return 0;

    LeaderboardEntry entry;
    DWORD read;
    while (count < max_entries && ReadFile(hFile, &entry, sizeof(LeaderboardEntry), &read, NULL) && read == sizeof(LeaderboardEntry)) {
        saves_decrypt(&entry, sizeof(LeaderboardEntry), LEADERBOARD_KEY, strlen(LEADERBOARD_KEY));
        if (entry.magic == LEADERBOARD_MAGIC) {
            size_t dataSize = offsetof(LeaderboardEntry, checksum);
            if (saves_checksum(&entry, dataSize) == entry.checksum) {
                out_entries[count++] = entry;
            }
        }
    }

    CloseHandle(hFile);
    return count;
}

void leaderboard_clear(AppState* /*app*/, GameMode mode) {
    const char* filename = get_leaderboard_file(mode);
    DeleteFileA(filename);
}