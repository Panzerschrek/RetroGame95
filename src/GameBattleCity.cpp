#include "GameBattleCity.hpp"
#include "GameMainMenu.hpp"
#include "Draw.hpp"

GameBattleCity::GameBattleCity(SoundPlayer& sound_player)
	: sound_player_(sound_player)
{
}

void GameBattleCity::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void) keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
	}
}

void GameBattleCity::Draw(const FrameBuffer frame_buffer) const
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	DrawText(frame_buffer, g_color_white, 0, 0, "battle city");
}

GameInterfacePtr GameBattleCity::AskForNextGameTransition()
{
	return std::move(next_game_);
}
