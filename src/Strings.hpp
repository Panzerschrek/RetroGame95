#pragma once

namespace Strings
{

#define DECLARE_STRING(x) extern const char x[];

DECLARE_STRING(window_title)

DECLARE_STRING(main_menu_new_game)
DECLARE_STRING(main_menu_continue_game)
DECLARE_STRING(main_menu_select_game)
DECLARE_STRING(main_menu_quit)

DECLARE_STRING(game_name_arkanoid)
DECLARE_STRING(game_name_tetris)
DECLARE_STRING(game_name_snake)
DECLARE_STRING(game_name_pacman)

DECLARE_STRING(arkanoid_round)
DECLARE_STRING(arkanoid_score)
DECLARE_STRING(arkanoid_game_over)

DECLARE_STRING(tetris_next)
DECLARE_STRING(tetris_level)
DECLARE_STRING(tetris_score)
DECLARE_STRING(tetris_game_over)

DECLARE_STRING(snake_level)
DECLARE_STRING(snake_length)
DECLARE_STRING(snake_lives)
DECLARE_STRING(snake_score)
DECLARE_STRING(snake_game_over)

DECLARE_STRING(pacman_level)
DECLARE_STRING(pacman_score)
DECLARE_STRING(pacman_ready)
DECLARE_STRING(pacman_game_over)

#undef DECLARE_STRING

} // namespace Strings
