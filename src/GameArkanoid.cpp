#include "GameArkanoid.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"

void GameArkanoid::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	(void)keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>();
		}
	}
}

void GameArkanoid::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);
}

GameInterfacePtr GameArkanoid::AskForNextGameTransition()
{
	return std::move(next_game_);
}
