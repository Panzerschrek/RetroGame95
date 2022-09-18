#pragma once
#include <cstdint>

enum class GameId
{
	Arkanoid,
	Tetris,
	Snake,
	Pacman,
	NumGames,
};

// This struct must be POD in order to save it into file directly.
struct Progress
{
	uint32_t opened_games_mask = 1; // First game is always opened.
	GameId current_game = GameId::Arkanoid;
};

Progress LoadProgress();

void OpenGame(GameId game_id);
