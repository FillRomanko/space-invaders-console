#ifndef SPACE_INVADERS_INPUT_H
#define SPACE_INVADERS_INPUT_H

#include "types.h"

// Коды клавиш (виртуальные коды Win32)
enum KeyCode : WORD {
    KEY_NONE      = 0,
    KEY_ESCAPE    = VK_ESCAPE,
    KEY_ENTER     = VK_RETURN,
    KEY_SPACE     = VK_SPACE,
    KEY_UP        = VK_UP,
    KEY_DOWN      = VK_DOWN,
    KEY_LEFT      = VK_LEFT,
    KEY_RIGHT     = VK_RIGHT,
    KEY_W         = 0x57,
    KEY_S         = 0x53,
    KEY_A         = 0x41,
    KEY_D         = 0x44,
    KEY_DELETE    = VK_DELETE,
    KEY_BACKSPACE = VK_BACK,
    KEY_COUNT     = 256, // для массива состояний
};

// Состояние ввода
struct InputState {
    // Нажатые в текущем кадре (key press events)
    uint8_t key_pressed[KEY_COUNT];

    // Удерживаемые (held down)
    uint8_t key_held[KEY_COUNT];

    int16_t mouse_x;
    int16_t mouse_y;
    uint8_t mouse_left;
    uint8_t mouse_right;
    uint8_t mouse_moved;

    // Текстовый ввод
    char input_chars[16];
    uint8_t input_char_count;
};

// Инициализация ввода
bool input_init(AppState* app);

// Опрос ввода (неблокирующий)
void input_poll(AppState* app, InputState* out);

// Проверка нажатия клавиши в текущем кадре (key down event)
uint8_t input_is_pressed(const InputState* state, KeyCode key);

// Проверка удержания клавиши (held down)
uint8_t input_is_held(const InputState* state, KeyCode key);

// Внутренняя: обновление состояния удержания (вызывается из input_poll)
void input_update_held_state(const InputState* current, InputState* held_state);

#endif // SPACE_INVADERS_INPUT_H