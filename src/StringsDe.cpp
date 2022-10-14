#include "Strings.hpp"

namespace Strings
{

// TODO - check these translations.

#define DEFINE_STRING(x, y) const char x[] = y;

DEFINE_STRING(window_title, "Vermischung")

DEFINE_STRING(paused, "angehalten")

DEFINE_STRING(main_menu_new_game, "Neues Spiel")
DEFINE_STRING(main_menu_continue_game, "Weiter spielen")
DEFINE_STRING(main_menu_select_game, "Spiel auswählen")
DEFINE_STRING(main_menu_quit, "Beenden")

DEFINE_STRING(game_name_arkanoid, "das Verlorenes Raumschiff")
DEFINE_STRING(game_name_tetris, "der Maurer")
DEFINE_STRING(game_name_snake, "die Gefreßige Schlange")
DEFINE_STRING(game_name_pacman, "Packmann")
DEFINE_STRING(game_name_battle_city, "Panzerkampf")
DEFINE_STRING(game_name_end_screen, "das Ende")

DEFINE_STRING(arkanoid_round, "Runde")
DEFINE_STRING(arkanoid_score, "Zahl")
DEFINE_STRING(arkanoid_game_over, "Spiel ist vorbei")
DEFINE_STRING(arkanoid_level_completed, "Level ist geschlossen!")

DEFINE_STRING(tetris_next, "Folgend")
DEFINE_STRING(tetris_level, "Level: ")
DEFINE_STRING(tetris_score, "Zahl: ")
DEFINE_STRING(tetris_game_over, "Spiel ist vorbei")
DEFINE_STRING(tetris_level_completed, "Level ist geschlossen!")

DEFINE_STRING(snake_level, "Level")
DEFINE_STRING(snake_length, "Länge")
DEFINE_STRING(snake_lives, "Leben")
DEFINE_STRING(snake_score, "Zahl")
DEFINE_STRING(snake_game_over, "Spiel ist vorbei")
DEFINE_STRING(snake_level_completed, "Level ist geschlossen!")

DEFINE_STRING(pacman_level, "Level")
DEFINE_STRING(pacman_score, "Zahl")
DEFINE_STRING(pacman_ready, "Sei bereit!")
DEFINE_STRING(pacman_game_over, "Spiel ist vorbei")
DEFINE_STRING(pacman_level_completed, "Level ist geschlossen!")

DEFINE_STRING(battle_city_stage, "STUFE")
DEFINE_STRING(battle_city_game_over, " SPIEL\n\n  IST\n\nVORBEI!")

DEFINE_STRING(end_screen_congratulations,
R"(
Gratulieren,

Sie haben das Spiel gewonnen!
)")

DEFINE_STRING(end_screen_authors,
R"(
Dieses Spiel wurde für den GameDev.ru

Retrospielwettbewerb (2022) gemacht.



Author: "Panzerschrek"
)")

DEFINE_STRING(end_screen_peace_text,
R"(
Es werde Frieden!

Fick den Krieg!
)")

#undef DEFINE_STRING

#undef DEFINE_STRING

} // namespace Strings
