#include "../../Include/core/sprites.h"
#include <cstring>

// Спрайт игрока (из player.txt) - 31 строка
const char* SPRITE_PLAYER[31] = {
    "       !       ",
    "       !       ",
    "       ^       ",
    "      / \\      ",
    "     /___\\     ",
    "    |=   =|    ",
    "    |     |    ",
    "    |     |    ",
    "    |     |    ",
    "    |     |    ",
    "    |     |    ",
    "    |     |    ",
    "    |     |    ",
    "    |     |    ",
    "    |     |    ",
    "   /|##!##|\\   ",
    "  / |##!##| \\  ",
    " /  |##!##|  \\ ",
    "|  / ^ | ^ \\  |",
    "| /  ( | )  \\ |",
    "|/   ( | )   \\|",
    "    ((   ))    ",
    "   ((  :  ))   ",
    "   ((  :  ))   ",
    "    ((   ))    ",
    "     (( ))     ",
    "      ( )      ",
    "       .       ",
    "       .       ",
    "       .       "
};
const int SPRITE_PLAYER_H = 31;
const int SPRITE_PLAYER_W = 15;

// Спрайт врага (из enemy.txt)
const char* SPRITE_ENEMY[18] = {
    "                _____                ",
    "             ,-\"     \"-.             ",
    "            / o       o \\            ",
    "           /   \\     /   \\           ",
    "          /     )-\"-(     \\          ",
    "         /     ( 6 6 )     \\         ",
    "        /       \\ \" /       \\        ",
    "       /         )=(         \\       ",
    "      /   o   .--\"-\"-.   o   \\      ",
    "     /    I  /  -   -  \\  I    \\     ",
    " .--(    (_}y/\\       /\\y{_)    )--.",
    "(    \".___l\\/__\\_____/__\\/l___,\"    )",
    " \\                                 /",
    "  \"-._      o O o O o O o      _,-",
    "      `--Y--.___________.--Y--'",
    "         |==.___________.==|",
    "         `==.___________.=='",
    ""
};
const int SPRITE_ENEMY_H = 17; // последняя строка пустая
const int SPRITE_ENEMY_W = 36;

// Спрайт зелёных деталей врага (из enemy_green.txt)
const char* SPRITE_ENEMY_GREEN[14] = {
    "",
    "",
    "              o       o",
    "               \\     /",
    "                )-\"\"-(",
    "               ( 6 6 )",
    "                \\ \" /",
    "                 )=(",
    "              .--\"-\"-.",
    "             /  -   -  \\",
    "         (_}y/\\       /\\y{_)",
    "           \\/  \\     /  \\/",
    "",
    ""
};
const int SPRITE_ENEMY_GREEN_H = 14;
const int SPRITE_ENEMY_GREEN_W = 22;

// Пуля - используем полный блок (█) в CP437 это 0xDB
const char SPRITE_BULLET = static_cast<char>(0xDB);

// Усиления (маленькие 3x3)
const char* SPRITE_POWERUP_SHIELD[3] = {
    " @ ",
    "@@ @",
    " @ "
};
const char* SPRITE_POWERUP_TRIPLE_STRAIGHT[3] = {
    " | ",
    "|||",
    " | "
};
const char* SPRITE_POWERUP_TRIPLE_DIAGONAL[3] = {
    "/|\\",
    " | ",
    "\\|/"
};
const int SPRITE_POWERUP_H = 3;
const int SPRITE_POWERUP_W = 3;