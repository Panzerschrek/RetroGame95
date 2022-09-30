#include "GameEndScreen.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "Sprites.hpp"

GameEndScreen::GameEndScreen(SoundPlayer& sound_player)
	: sound_player_(sound_player)
{
	OpenGame(GameId::EndScreen);
}

void GameEndScreen::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
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

void GameEndScreen::Draw(const FrameBuffer frame_buffer) const
{
	DrawSprite(frame_buffer, Sprites::dresdner_zwinger_in_der_nacht, 0, 0);

	DrawText(frame_buffer, g_color_white, 0, 0, "The end");
}

GameInterfacePtr GameEndScreen::AskForNextGameTransition()
{
	return std::move(next_game_);
}
