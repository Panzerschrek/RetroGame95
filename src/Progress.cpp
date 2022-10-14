#include "Progress.hpp"
#include <cstdio>

namespace
{

const char g_file_name[] = "save.bin";

void SaveProgress(const Progress& progress)
{
	FILE* const f = std::fopen(g_file_name, "wb");
	if(f == nullptr)
	{
		return;
	}

	std::fwrite(&progress, sizeof(Progress), 1, f);
	std::fclose(f);
}

} // namespace

GameId Progress::GetLastOpenedGame() const
{
	uint32_t last_opened_game = 0;
	for(uint32_t i = 0; i < uint32_t(GameId::NumGames); ++i)
	{
		if(((1 << i) & opened_games_mask) != 0)
		{
			last_opened_game = i;
		}
	}

	return GameId(last_opened_game);
}

uint32_t Progress::GetNumOpenedGames() const
{
	return uint32_t(GetLastOpenedGame()) + 1;
}

Progress LoadProgress()
{
	Progress progress;

	FILE* const f = std::fopen(g_file_name, "rb");
	if(f == nullptr)
	{
		return progress;
	}

	std::fread(&progress, sizeof(Progress), 1, f);
	std::fclose(f);
	return progress;
}

void OpenGame(const GameId game_id)
{
	Progress progress = LoadProgress();
	progress.opened_games_mask |= 1 << uint32_t(game_id);
	progress.current_game = game_id;
	SaveProgress(progress);
}
