#ifndef SPACE_INVADERS_TYPES_H
#define SPACE_INVADERS_TYPES_H

#include <cstdint>
#include <windows.h>

// ============================================================
// Константы и ограничения памяти
// ============================================================
static constexpr int MAX_PLAY_WIDTH  = 320;  // игровая область (внутри рамки) ~300
static constexpr int MAX_PLAY_HEIGHT = 180;  // ~150
static constexpr int MAX_BULLETS        = 112;  // 7 массивов по 16 пуль
static constexpr int MAX_ENEMIES        = 64;
static constexpr int MAX_POWERUPS       = 32;
static constexpr int MAX_SAVES          = 16;
static constexpr int MAX_LEADERBOARD    = 10;
static constexpr int MAX_NAME_LEN       = 32;
static constexpr int MAX_PATH_LEN       = 260;
static constexpr int SAVE_MAGIC         = 0x53504349; // "ISPC" в little-endian
static constexpr int LEADERBOARD_MAGIC  = 0x4253484C; // "LHSB"

// ASCII Font sprite dimensions
static constexpr int FONT_CHAR_W = 6;  // ширина символа в колонках
static constexpr int FONT_CHAR_H = 6;  // высота символа в строках
static constexpr int FONT_CHARS_PER_ROW = 16; // 16 символов в строке атласа
static constexpr int FONT_TOTAL_CHARS = 96;   // 32..127 (printable ASCII)

// ============================================================
// Цвета (Win32 консольные атрибуты) - ЧЁРНЫЙ ФОН
// ============================================================
enum Color : WORD {
    COL_BLACK       = 0x0000,
    COL_DARK_BLUE   = 0x0001,
    COL_DARK_GREEN  = 0x0002,
    COL_DARK_CYAN   = 0x0003,
    COL_DARK_RED    = 0x0004,
    COL_DARK_MAGENTA= 0x0005,
    COL_DARK_YELLOW = 0x0006,
    COL_GRAY        = 0x0007,
    COL_DARK_GRAY   = 0x0008,
    COL_BLUE        = 0x0009,
    COL_GREEN       = 0x000A,
    COL_CYAN        = 0x000B,
    COL_RED         = 0x000C,
    COL_MAGENTA     = 0x000D,
    COL_YELLOW      = 0x000E,
    COL_WHITE       = 0x000F,

    // Комбинированные для ЧЁРНОГО фона
    COL_MENU_BG     = COL_BLACK << 4,              // чёрный фон меню
    COL_MENU_SEL    = COL_YELLOW | (COL_BLACK << 4), // жёлтый на чёрном (выбранное)
    COL_MENU_NORM   = COL_GREEN  | (COL_BLACK << 4), // зелёный на чёрном (обычное)
    COL_GAME_BG     = COL_BLACK << 4,              // чёрный фон игры
    COL_PLAYER      = COL_YELLOW | (COL_BLACK << 4), // жёлтый игрок на чёрном
    COL_ENEMY       = COL_RED    | (COL_BLACK << 4), // красный враг на чёрном
    COL_ENEMY_GREEN = COL_GREEN  | (COL_BLACK << 4), // зелёные детали врага на чёрном
    COL_BULLET      = COL_WHITE  | (COL_BLACK << 4), // белая пуля на чёрном
    COL_POWERUP     = COL_CYAN   | (COL_BLACK << 4), // голубое усиление на чёрном
    COL_UI_TEXT     = COL_WHITE  | (COL_BLACK << 4), // белый текст UI на чёрном
    COL_UI_GREEN    = COL_GREEN  | (COL_BLACK << 4), // зелёный текст UI на чёрном
    COL_UI_YELLOW   = COL_YELLOW | (COL_BLACK << 4), // жёлтый текст UI на чёрном
    COL_UI_RED      = COL_RED    | (COL_BLACK << 4), // красный текст UI на чёрном
    COL_FRAME       = COL_WHITE  | (COL_BLACK << 4), // рамка белая на чёрном
};

// ============================================================
// Типы пуль
// ============================================================
enum BulletType : uint8_t {
    BULLET_NONE   = 0,
    BULLET_UP     = 1,    // вертикально вверх
    BULLET_UP_RIGHT = 2,  // вверх-вправо
    BULLET_UP_LEFT  = 3,  // вверх-влево
};

// ============================================================
// Типы усилений
// ============================================================
enum PowerUpType : uint8_t {
    POWERUP_NONE   = 0,
    POWERUP_SHIELD = 1,   // щит
    POWERUP_TRIPLE_STRAIGHT = 2, // тройные прямые
    POWERUP_TRIPLE_DIAGONAL = 3, // тройные диагональные
};

// ============================================================
// Режимы игры
// ============================================================
enum GameMode : uint8_t {
    MODE_NORMAL  = 0,
    MODE_DEFENSE = 1,
};

// ============================================================
// Состояния меню
// ============================================================
enum MenuState : uint8_t {
    MENU_MAIN       = 0,
    MENU_PLAY_NORMAL= 1,
    MENU_PLAY_DEFENSE = 2,
    MENU_HIGHSCORES = 3,
    MENU_SAVES      = 4,
    MENU_CHANGE_NAME= 5,
    MENU_EXIT       = 6,
    MENU_PAUSE      = 7,
    MENU_GAME_OVER  = 8,
};

// ============================================================
// Структуры данных (POD, без STL)
// ============================================================

// Координата со знаком (int16_t: -32768..32767) - для 300x150
struct Vec2 {
    int16_t x;
    int16_t y;
};

// Прямоугольник для хитбоксов
struct Rect {
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
};

// Пуля (в массивах по типам)
struct Bullet {
    int16_t x;
    int16_t y;
    uint8_t active; // 0/1
};

// Враг
struct Enemy {
    int16_t x;
    int16_t y;
    uint8_t active;
    uint8_t frame; // для анимации (если понадобится)
};

// Усиление
struct PowerUp {
    int16_t x;
    int16_t y;
    uint8_t type; // PowerUpType
    uint8_t active;
};

// Игрок
struct Player {
    int16_t x;
    int16_t y;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t shield_active;      // 0/1
    uint8_t shield_hitbox_extra; // +1 к хитбоксу при щите
    uint16_t triple_straight_timer; // тики до конца (0 = нет)
    uint16_t triple_diagonal_timer; // тики до конца
    uint32_t score;
    uint8_t shoot_cooldown;     // кулдаун стрельбы (в кадрах)
};

// Спрайт ASCII шрифта (6x6 на символ)
struct FontChar {
    char data[FONT_CHAR_H][FONT_CHAR_W];
};

// Атлас шрифта
struct FontAtlas {
    FontChar chars[FONT_TOTAL_CHARS]; // 32..127
    uint8_t loaded;
};

// Сохранение игры
struct SaveSlot {
    uint32_t magic;           // SAVE_MAGIC
    uint32_t version;         // версия формата
    uint64_t timestamp;       // время сохранения (FILETIME)
    char player_name[MAX_NAME_LEN];
    GameMode mode;
    Player player;
    uint16_t enemy_spawn_timer;
    uint16_t enemy_spawn_interval;
    uint32_t game_time_ticks; // время игры в тиках
    uint16_t base_hp;         // для MODE_DEFENSE
    uint16_t base_max_hp;
    uint8_t enemy_count;
    Enemy enemies[MAX_ENEMIES];
    uint8_t bullet_up_count;
    Bullet bullets_up[MAX_BULLETS / 7 * 3]; // примерно
    uint8_t bullet_ur_count;
    Bullet bullets_ur[MAX_BULLETS / 7 * 2];
    uint8_t bullet_ul_count;
    Bullet bullets_ul[MAX_BULLETS / 7 * 2];
    uint8_t powerup_count;
    PowerUp powerups[MAX_POWERUPS];
    uint32_t checksum;        // простая контрольная сумма
};

// Запись таблицы рекордов
struct LeaderboardEntry {
    uint32_t magic;           // LEADERBOARD_MAGIC
    char name[MAX_NAME_LEN];
    uint32_t score;
    uint64_t timestamp;
    GameMode mode;
    uint32_t checksum;
};

// Игровое состояние (всё в одном месте для простоты сохранения)
struct GameState {
    // Настройки консоли
    int play_width;
    int play_height;
    int offset_x;
    int offset_y;

    // Текущий режим
    GameMode mode;

    // Игрок
    Player player;

    // Враги
    Enemy enemies[MAX_ENEMIES];
    uint8_t enemy_count;
    uint16_t enemy_spawn_timer;
    uint16_t enemy_spawn_interval; // динамический
    uint16_t enemy_speed_num;      // числитель скорости (fixed point)
    uint16_t enemy_speed_den;      // знаменатель
    uint32_t game_time_ticks;

    // Пули (3 массива по типам)
    Bullet bullets_up[48];    // вертикальные
    Bullet bullets_ur[32];    // вверх-вправо
    Bullet bullets_ul[32];    // вверх-влево
    uint8_t bullet_up_count;
    uint8_t bullet_ur_count;
    uint8_t bullet_ul_count;

    // Усиления
    PowerUp powerups[MAX_POWERUPS];
    uint8_t powerup_count;

    // База (для MODE_DEFENSE)
    uint16_t base_hp;
    uint16_t base_max_hp;

    // Имя игрока
    char player_name[MAX_NAME_LEN];

    // Флаги
    uint8_t paused;
    uint8_t game_over;
    uint8_t in_menu;
    uint8_t show_save_preview; // при загрузке сохранения - показать поле и ждать нажатия

    // Шрифт
    FontAtlas font;
};

// Глобальное состояние приложения
struct AppState {
    // Консоль
    HANDLE hConsole;
    HANDLE hBuffer[2];       // двойная буферизация
    int activeBuffer;
    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    CONSOLE_FONT_INFOEX fontInfo;
    int consoleWidth;
    int consoleHeight;
    int playWidth;
    int playHeight;
    int offsetX;
    int offsetY;

    // Ввод
    HANDLE hInput;
    INPUT_RECORD inputBuffer[32];
    DWORD inputEvents;

    // Игра
    GameState game;
    MenuState menuState;
    int menuSelection;
    int menuItemCount;

    // Аудио
    // Будем использовать PlaySound из winmm.lib

    // Тайминг
    uint64_t lastFrameTime;
    uint64_t frameCount;
    uint32_t ticksPerSecond; // 60

    // Автосохранение
    uint32_t autosaveTimer;

    // Музыка
    uint8_t currentMusicTrack;

    // Имя игрока (по умолчанию "player1")
    char playerName[MAX_NAME_LEN];

    // Флаг выхода из программы
    uint8_t shouldExit;

    // Текущий режим шрифта консоли (для отслеживания смены меню/игра)
    uint8_t currentFontMode; // 0 = FONT_MODE_MENU, 1 = FONT_MODE_GAME
};

#endif // SPACE_INVADERS_TYPES_H