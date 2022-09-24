#include "Strings.hpp"

namespace Strings
{

#define DEFINE_STRING(x, y) const char x[] = y;

DEFINE_STRING(main_menu_game_title, "RetroGame95")
DEFINE_STRING(main_menu_new_game, "New game")
DEFINE_STRING(main_menu_continue_game, "Continue game")
DEFINE_STRING(main_menu_select_game, "Select game")
DEFINE_STRING(main_menu_quit, "Quit")

DEFINE_STRING(game_name_arkanoid, "Arkanoid")
DEFINE_STRING(game_name_tetris, "Tetris")
DEFINE_STRING(game_name_snake, "Snake")
DEFINE_STRING(game_name_pacman, "Pacman")

DEFINE_STRING(arkanoid_round, "Round")
DEFINE_STRING(arkanoid_score, "Score")
DEFINE_STRING(arkanoid_game_over, "Game Over")

DEFINE_STRING(tetris_next, "Next")
DEFINE_STRING(tetris_level, "Level: ")
DEFINE_STRING(tetris_score, "Score: ")
DEFINE_STRING(tetris_game_over, "Game Over")

DEFINE_STRING(snake_level, "level")
DEFINE_STRING(snake_length, "length")
DEFINE_STRING(snake_lives, "lifes")
DEFINE_STRING(snake_score, "score")
DEFINE_STRING(snake_game_over, "game over")

DEFINE_STRING(pacman_level, "level")
DEFINE_STRING(pacman_score, "score")
DEFINE_STRING(pacman_ready, "ready!")
DEFINE_STRING(pacman_game_over, "game over")

#undef DEFINE_STRING

} // namespace Strings
