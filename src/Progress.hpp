#pragma once
#include <cstdint>

enum class GameId
{
	Arkanoid,
	Tetris,
	Snake,
	Pacman,
};

struct Progress
{
	uint32_t opened_games_mask = 1; // First game is always opened.
	GameId current_game = GameId::Arkanoid;
};

Progress LoadProgress();

void OpenGame(GameId game_id);
