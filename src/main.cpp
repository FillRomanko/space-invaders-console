#include "utils/console_utils.h"
#include "core/types.h"
#include "core/renderer.h"
#include "core/input.h"
#include "core/audio.h"
#include "core/game.h"
#include "core/menu.h"
#include "core/save_system.h"
#include "core/leaderboard.h"
#include <windows.h>
#include <cstdio>
#include <cstring>

// ============================================================
// Главная точка входа
// ============================================================
int main() {
    AppState app;
    std::memset(&app, 0, sizeof(AppState));

    // Имя игрока по умолчанию
    strcpy_s(app.playerName, "player1");

    // 1. Инициализация консоли (полноэкранный режим, подбор шрифта, рамка)
    // Используем размеры игрового поля ~300x150 внутри рамки
    const int PLAY_WIDTH = 300;
    const int PLAY_HEIGHT = 150;

    if (!console_utils::InitializeConsole(PLAY_WIDTH + 2, PLAY_HEIGHT + 2)) {
        return 1;
    }

    // Получаем хэндл консоли после инициализации
    app.hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Получаем информацию о буфере
    GetConsoleScreenBufferInfo(app.hConsole, &app.bufferInfo);
    app.consoleWidth = app.bufferInfo.srWindow.Right - app.bufferInfo.srWindow.Left + 1;
    app.consoleHeight = app.bufferInfo.srWindow.Bottom - app.bufferInfo.srWindow.Top + 1;

    // Вычисляем размеры игровой области и смещения
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(app.hConsole, &info);

    int winCols = info.srWindow.Right - info.srWindow.Left + 1;
    int winRows = info.srWindow.Bottom - info.srWindow.Top + 1;

    // Use actual console size for play area (minus 2 for border)
    int actualPlayWidth = winCols - 2;
    int actualPlayHeight = winRows - 2;
    if (actualPlayWidth < 10) actualPlayWidth = 10;
    if (actualPlayHeight < 10) actualPlayHeight = 10;

    app.offsetX = 1;  // Border at left
    app.offsetY = 1;  // Border at top

    app.playWidth = actualPlayWidth;
    app.playHeight = actualPlayHeight;

    // 2. Инициализация подсистем
    if (!renderer_init(&app)) {
        return 1;
    }

    if (!input_init(&app)) {
        return 1;
    }

    if (!audio_init(&app)) {
        // Аудио не критично, продолжаем
    }

    if (!saves_init(&app)) {
        // Сохранения не критичны
    }

    if (!leaderboard_init(&app)) {
        // Лидерборд не критичен
    }

    // 3. Начальное состояние - главное меню
    app.menuState = MENU_MAIN;
    app.menuSelection = 0;
    app.menuItemCount = MAIN_COUNT;
    app.game.in_menu = 1;
    app.game.mode = MODE_NORMAL;
    app.shouldExit = 0;

    // Запускаем музыку меню
    audio_play_music(&app, MUS_MENU_1);

    // 4. Основной игровой цикл
    InputState input;
    LARGE_INTEGER freq, lastTime, currentTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);

    const double targetFrameTime = 1.0 / 60.0; // 60 FPS
    bool shouldExit = false;

    while (!shouldExit) {
        QueryPerformanceCounter(&currentTime);
        double dt = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / static_cast<double>(freq.QuadPart);

        // Ограничение FPS
        if (dt < targetFrameTime) {
            Sleep(static_cast<DWORD>((targetFrameTime - dt) * 1000.0));
            QueryPerformanceCounter(&currentTime);
            dt = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / static_cast<double>(freq.QuadPart);
        }
        lastTime = currentTime;

        // Опрос ввода
        input_poll(&app, &input);

        // Проверка на выход
        if (app.shouldExit) {
            shouldExit = true;
            break;
        }

        if (app.game.in_menu) {
            // Обработка меню
            menu_handle_input(&app, &input);
            menu_update(&app);
        } else {
            // Игровой тик
            game_tick(&app, &input, static_cast<float>(dt));

            // Проверка показа превью сохранения
            if (app.game.show_save_preview) {
                if (input_is_pressed(&input, KEY_ENTER) || input_is_pressed(&input, KEY_SPACE) || input_is_pressed(&input, KEY_ESCAPE) || input.mouse_left) {
                    app.game.show_save_preview = 0;
                }
            }

            // Проверка Game Over
            if (app.game.game_over) {
                app.menuState = MENU_GAME_OVER;
                app.menuSelection = 0;
                app.menuItemCount = GO_COUNT;
                app.game.in_menu = 1;
                audio_stop_music(&app);
            }
        }

        // Рендеринг
        renderer_clear(&app, COL_GAME_BG);
        renderer_draw_frame(&app);

        if (app.game.in_menu) {
            switch (app.menuState) {
                case MENU_MAIN:
                    renderer_draw_menu(&app);
                    break;
                case MENU_PAUSE:
                    // Рисуем игру под меню паузы
                    renderer_draw_player(&app);
                    renderer_draw_enemies(&app);
                    renderer_draw_bullets(&app);
                    renderer_draw_powerups(&app);
                    renderer_draw_ui(&app);
                    renderer_draw_pause(&app);
                    break;
                case MENU_GAME_OVER:
                    renderer_draw_game_over(&app);
                    break;
                case MENU_HIGHSCORES:
                    renderer_draw_leaderboard(&app, app.game.mode);
                    break;
                case MENU_SAVES:
                    renderer_draw_saves(&app);
                    break;
                case MENU_CHANGE_NAME:
                    renderer_draw_change_name(&app);
                    break;
                case MENU_PLAY_NORMAL:
                case MENU_PLAY_DEFENSE:
                case MENU_EXIT:
                    // Эти состояния не должны достигаться в рендеринге (переход в игру/выход)
                    break;
                default:
                    break;
            }
        } else {
            // Игровая сцена
            renderer_draw_player(&app);
            renderer_draw_enemies(&app);
            renderer_draw_bullets(&app);
            renderer_draw_powerups(&app);
            renderer_draw_ui(&app);

            if (app.game.show_save_preview) {
                renderer_draw_save_preview(&app);
            }
        }

        // Презентация кадра
        renderer_present(&app);

        app.frameCount++;
    }

    // 5. Очистка
    audio_cleanup(&app);

    // Восстанавливаем консоль
    SetConsoleActiveScreenBuffer(app.hConsole);

    return 0;
}