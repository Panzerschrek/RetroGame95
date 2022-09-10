#include "GameMainMenu.hpp"
#include "Draw.hpp"
#include "GameArkanoid.hpp"
#include "GameTetris.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"
#include <SDL_keyboard.h>
#include <cassert>

namespace
{

GameInterfacePtr CreateGameByIndex(const uint32_t index)
{
	switch(index)
	{
	case 0: return std::make_unique<GameArkanoid>();
	case 1: return std::make_unique<GameTetris>();
	}

	return nullptr;
}

} // namespace

void GameMainMenu::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	(void)keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN)
		{
			if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				quit_triggered_ = true;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_RETURN)
			{
				switch(current_row_)
				{
				case MenuRow::NewGame:
					if(next_game_ == nullptr)
					{
						next_game_ = CreateGameByIndex(0);
					}
					break;

				case MenuRow::Quit:
					quit_triggered_ = true;
					break;

				case MenuRow::NumRows:
					assert(false);
					break;
				}
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_UP)
			{
				current_row_ =
					MenuRow((uint32_t(current_row_) + uint32_t(MenuRow::NumRows) - 1) % uint32_t(MenuRow::NumRows));
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_DOWN)
			{
				current_row_ = MenuRow((uint32_t(current_row_) + 1) % uint32_t(MenuRow::NumRows));
			}
			if(event.key.keysym.scancode >= SDL_SCANCODE_F1 && event.key.keysym.scancode <= SDL_SCANCODE_F12)
			{
				// TODO - consider this cheat.
				if(next_game_ == nullptr)
				{
					next_game_ = CreateGameByIndex(event.key.keysym.scancode - SDL_SCANCODE_F1);
				}
			}
		}
	}
}

void GameMainMenu::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t title_offset_x = 104;
	const uint32_t title_offset_y = 32;
	const uint32_t offset_x = 112;
	const uint32_t offset_y = 96;

	DrawText(
		frame_buffer,
		g_color_white,
		title_offset_x,
		title_offset_y,
		"RetroGame95");

	const char* const texts[] = {"New game", "Quit"};
	for(size_t i = 0; i < std::size(texts); ++i)
	{
		DrawText(
			frame_buffer,
			g_color_white,
			offset_x,
			offset_y + 16 * uint32_t(i),
			texts[i]);
	}

	DrawText(
		frame_buffer,
		g_color_white,
		offset_x - 16,
		offset_y + 16 * uint32_t(current_row_),
		">");
}

GameInterfacePtr GameMainMenu::AskForNextGameTransition()
{
	return std::move(next_game_);
}

bool GameMainMenu::AskForQuit()
{
	return quit_triggered_;
}
