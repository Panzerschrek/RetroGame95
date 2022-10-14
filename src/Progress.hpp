#pragma once
#include <cstdint>

enum class GameId
{
	Arkanoid,
	Tetris,
	Snake,
	Pacman,
	BattleCity,
	EndScreen,
	NumGames,
};

// This struct must be POD in order to save it into file directly.
struct Progress
{
	uint32_t opened_games_mask = 1; // First game is always opened.
	GameId current_game = GameId::Arkanoid;

	GameId GetLastOpenedGame() const;
	uint32_t GetNumOpenedGames() const;
};

Progress LoadProgress();

void OpenGame(GameId game_id);
