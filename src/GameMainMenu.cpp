#include "GameMainMenu.hpp"
#include "Draw.hpp"
#include "GameTetris.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"
#include <SDL_keyboard.h>

void GameMainMenu::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	(void) events;

	if(next_game_ == nullptr && SDL_SCANCODE_RETURN < keyboard_state.size() && keyboard_state[SDL_SCANCODE_RETURN])
	{
		next_game_ = std::make_unique<GameTetris>();
	}
}

void GameMainMenu::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::gimp_harold), 47, 16);

	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_ghost), 128, 32 );
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_1), 144, 32);
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_food), 144 + 16, 32 + 6);
}

GameInterfacePtr GameMainMenu::AskForNextGameTransition()
{
	return std::move(next_game_);
}
