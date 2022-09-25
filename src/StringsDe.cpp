#include "Strings.hpp"

namespace Strings
{

// TODO - check these translations.

#define DEFINE_STRING(x, y) const char x[] = y;

DEFINE_STRING(window_title, "AltesSpiel95")

DEFINE_STRING(main_menu_new_game, "Neues Spiel")
DEFINE_STRING(main_menu_continue_game, "weiter spielen")
DEFINE_STRING(main_menu_select_game, "Spiel auswählen")
DEFINE_STRING(main_menu_quit, "Beenden")

DEFINE_STRING(game_name_arkanoid, "das Verlorenes Raumschiff")
DEFINE_STRING(game_name_tetris, "der Maurer")
DEFINE_STRING(game_name_snake, "die Gefreßige Schlange")
DEFINE_STRING(game_name_pacman, "Packmann")

DEFINE_STRING(arkanoid_round, "Runde")
DEFINE_STRING(arkanoid_score, "Zahl")
DEFINE_STRING(arkanoid_game_over, "Spiel ist vorbei")

DEFINE_STRING(tetris_next, "Folgend")
DEFINE_STRING(tetris_level, "Level: ")
DEFINE_STRING(tetris_score, "Zahl: ")
DEFINE_STRING(tetris_game_over, "Spiel ist vorbei")

DEFINE_STRING(snake_level, "Level")
DEFINE_STRING(snake_length, "Länge")
DEFINE_STRING(snake_lives, "Leben")
DEFINE_STRING(snake_score, "Zahl")
DEFINE_STRING(snake_game_over, "Spiel ist vorbei")

DEFINE_STRING(pacman_level, "Level")
DEFINE_STRING(pacman_score, "Zahl")
DEFINE_STRING(pacman_ready, "Sei bereit!")
DEFINE_STRING(pacman_game_over, "Spiel ist vorbei")

#undef DEFINE_STRING

} // namespace Strings