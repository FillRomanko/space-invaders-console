#include "../../Include/core/menu.h"
#include "../../Include/core/game.h"
#include "../../Include/core/types.h"
#include "../../Include/core/audio.h"
#include "../../Include/core/save_system.h"
#include "../../Include/core/leaderboard.h"
#include "../../Include/core/input.h"
#include <cstring>
#include <cstdio>

// Временный буфер для ввода имени
char g_nameInput[MAX_NAME_LEN];
int g_nameInputLen = 0;

void menu_draw_main(AppState* /*app*/) {
    // Рисуется в renderer_draw_menu
}

void menu_draw_pause(AppState* /*app*/) {
    // Рисуется в renderer_draw_pause
}

void menu_draw_game_over(AppState* /*app*/) {
    // Рисуется в renderer_draw_game_over
}

void menu_draw_highscores(AppState* /*app*/) {
    // Рисуется в renderer_draw_leaderboard
}

void menu_draw_saves(AppState* /*app*/) {
    // Рисуется в renderer_draw_saves
}

void menu_draw_change_name(AppState* /*app*/) {
    // Рисуется в renderer_draw_change_name
}

void menu_handle_main(AppState* app, const InputState* input) {
    // Навигация - нажатие в текущем кадре
    if (input_is_pressed(input, KEY_UP) || input_is_pressed(input, KEY_W)) {
        if (app->menuSelection > 0) {
            app->menuSelection--;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }
    if (input_is_pressed(input, KEY_DOWN) || input_is_pressed(input, KEY_S)) {
        if (app->menuSelection < app->menuItemCount - 1) {
            app->menuSelection++;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }

    // Выбор
    if (input_is_pressed(input, KEY_ENTER) || input_is_pressed(input, KEY_SPACE)) {
        audio_play_sound(app, SND_MENU_SELECT);

        switch (app->menuSelection) {
            case MAIN_PLAY_NORMAL:
                game_start(app, MODE_NORMAL);
                break;
            case MAIN_PLAY_DEFENSE:
                game_start(app, MODE_DEFENSE);
                break;
            case MAIN_HIGHSCORES:
                app->menuState = MENU_HIGHSCORES;
                app->menuSelection = 0;
                app->menuItemCount = 1; // только выход
                break;
            case MAIN_SAVES:
                app->menuState = MENU_SAVES;
                app->menuSelection = 0;
                // menuItemCount обновится в saves_get_list
                break;
            case MAIN_CHANGE_NAME:
                app->menuState = MENU_CHANGE_NAME;
                app->menuSelection = 0;
                g_nameInputLen = 0;
                memset(g_nameInput, 0, MAX_NAME_LEN);
                break;
            case MAIN_EXIT:
                app->shouldExit = 1;
                break;
        }
    }
}

void menu_handle_pause(AppState* app, const InputState* input) {
    if (input_is_pressed(input, KEY_UP) || input_is_pressed(input, KEY_W)) {
        if (app->menuSelection > 0) {
            app->menuSelection--;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }
    if (input_is_pressed(input, KEY_DOWN) || input_is_pressed(input, KEY_S)) {
        if (app->menuSelection < app->menuItemCount - 1) {
            app->menuSelection++;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }

    if (input_is_pressed(input, KEY_ENTER) || input_is_pressed(input, KEY_SPACE)) {
        audio_play_sound(app, SND_MENU_SELECT);

        switch (app->menuSelection) {
            case PAUSE_RESUME:
                game_toggle_pause(app);
                break;
            case PAUSE_MAIN_MENU:
                game_exit_to_menu(app);
                break;
            case PAUSE_EXIT_GAME:
                app->shouldExit = 1;
                break;
        }
    }

    // ESC также закрывает паузу
    if (input_is_pressed(input, KEY_ESCAPE)) {
        game_toggle_pause(app);
    }
}

void menu_handle_game_over(AppState* app, const InputState* input) {
    if (input_is_pressed(input, KEY_UP) || input_is_pressed(input, KEY_W)) {
        if (app->menuSelection > 0) {
            app->menuSelection--;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }
    if (input_is_pressed(input, KEY_DOWN) || input_is_pressed(input, KEY_S)) {
        if (app->menuSelection < app->menuItemCount - 1) {
            app->menuSelection++;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }

    if (input_is_pressed(input, KEY_ENTER) || input_is_pressed(input, KEY_SPACE)) {
        audio_play_sound(app, SND_MENU_SELECT);

        switch (app->menuSelection) {
            case GO_RESTART:
                game_start(app, app->game.mode);
                break;
            case GO_MAIN_MENU:
                game_exit_to_menu(app);
                break;
            case GO_EXIT:
                app->shouldExit = 1;
                break;
        }
    }
}

void menu_handle_highscores(AppState* app, const InputState* input) {
    if (input_is_pressed(input, KEY_ESCAPE) || input_is_pressed(input, KEY_ENTER) || input_is_pressed(input, KEY_SPACE)) {
        audio_play_sound(app, SND_MENU_SELECT);
        game_exit_to_menu(app);
    }
}

void menu_handle_saves(AppState* app, const InputState* input) {
    SaveSlot saves[MAX_SAVES];
    int count = saves_get_list(app, saves, MAX_SAVES);
    app->menuItemCount = count > 0 ? count : 1;

    if (input_is_pressed(input, KEY_UP) || input_is_pressed(input, KEY_W)) {
        if (app->menuSelection > 0) {
            app->menuSelection--;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }
    if (input_is_pressed(input, KEY_DOWN) || input_is_pressed(input, KEY_S)) {
        if (app->menuSelection < app->menuItemCount - 1) {
            app->menuSelection++;
            audio_play_sound(app, SND_MENU_MOVE);
        }
    }

    if ((input_is_pressed(input, KEY_ENTER) || input_is_pressed(input, KEY_SPACE)) && count > 0) {
        audio_play_sound(app, SND_MENU_SELECT);
        // Загружаем сохранение
        if (saves_load_slot(app, app->menuSelection)) {
            // Игра запустится из превью в game_tick
            app->game.show_save_preview = 1;
        }
    }

    // Удаление сохранения (Delete ключ - VK_DELETE = 0x2E)
    if (input_is_pressed(input, KEY_DELETE) && count > 0) {
        audio_play_sound(app, SND_MENU_SELECT);
        saves_delete_slot(app, app->menuSelection);
        // После удаления обновляем список
        if (app->menuSelection >= count - 1 && app->menuSelection > 0) {
            app->menuSelection--;
        }
    }

    if (input_is_pressed(input, KEY_ESCAPE)) {
        audio_play_sound(app, SND_MENU_SELECT);
        game_exit_to_menu(app);
    }
}

void menu_handle_change_name(AppState* app, const InputState* input) {
    if (input_is_pressed(input, KEY_ESCAPE)) {
        audio_play_sound(app, SND_MENU_SELECT);
        g_nameInputLen = 0;
        game_exit_to_menu(app);
        return;
    }

    if (input_is_pressed(input, KEY_ENTER) && g_nameInputLen > 0) {
        audio_play_sound(app, SND_MENU_SELECT);
        strncpy_s(app->playerName, g_nameInput, MAX_NAME_LEN);
        strncpy_s(app->game.player_name, g_nameInput, MAX_NAME_LEN);
        g_nameInputLen = 0;
        game_exit_to_menu(app);
        return;
    }

    // Обработка Backspace
    if (input_is_pressed(input, KEY_BACKSPACE) && g_nameInputLen > 0) {
        g_nameInputLen--;
        g_nameInput[g_nameInputLen] = 0;
        audio_play_sound(app, SND_MENU_MOVE);
    }

    // Обработка введенных символов
    for (uint8_t i = 0; i < input->input_char_count; ++i) {
        if (g_nameInputLen < MAX_NAME_LEN - 1) {
            g_nameInput[g_nameInputLen++] = input->input_chars[i];
            g_nameInput[g_nameInputLen] = 0;
        }
    }
}

void menu_update(AppState* /*app*/) {
    // Обновление состояния меню (анимации и т.д.)
    // Пока пусто
}

void menu_handle_input(AppState* app, const InputState* input) {
    switch (app->menuState) {
        case MENU_MAIN:
            menu_handle_main(app, input);
            break;
        case MENU_PAUSE:
            menu_handle_pause(app, input);
            break;
        case MENU_GAME_OVER:
            menu_handle_game_over(app, input);
            break;
        case MENU_HIGHSCORES:
            menu_handle_highscores(app, input);
            break;
        case MENU_SAVES:
            menu_handle_saves(app, input);
            break;
        case MENU_CHANGE_NAME:
            menu_handle_change_name(app, input);
            break;
        case MENU_PLAY_NORMAL:
        case MENU_PLAY_DEFENSE:
        case MENU_EXIT:
            // Эти состояния не должны обрабатываться здесь (переход в игру/выход)
            break;
    }
}