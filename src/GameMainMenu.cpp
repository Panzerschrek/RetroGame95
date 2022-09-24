#include "GameMainMenu.hpp"
#include "Draw.hpp"
#include "GameArkanoid.hpp"
#include "GamePacman.hpp"
#include "GameSnake.hpp"
#include "GameTetris.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"
#include <SDL_keyboard.h>
#include <cassert>

namespace
{

GameInterfacePtr CreateGameById(const GameId id, SoundPlayer& sound_player)
{
	switch(id)
	{
	case GameId::Arkanoid: return std::make_unique<GameArkanoid>(sound_player);
	case GameId::Tetris: return std::make_unique<GameTetris>(sound_player);
	case GameId::Snake: return std::make_unique<GameSnake>(sound_player);
	case GameId::Pacman: return std::make_unique<GamePacman>(sound_player);
	case GameId::NumGames: break;
	}

	assert(false);
	return nullptr;
}

GameInterfacePtr CreateGameByIndex(const uint32_t index, SoundPlayer& sound_player)
{
	return CreateGameById(GameId(index), sound_player);
}

} // namespace

GameMainMenu::GameMainMenu(SoundPlayer& sound_player)
	: sound_player_(sound_player), progress_(LoadProgress())
{
}

void GameMainMenu::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void)keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN)
		{
			if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				if(std::get_if<MenuRow>(&current_row_) != nullptr)
				{
					quit_triggered_ = true;
				}
				else if( std::get_if<SelectGameMenuRow>(&current_row_) != nullptr)
				{
					current_row_ = MenuRow::SelectGame;
				}
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_RETURN)
			{
				if(const auto main_menu_row = std::get_if<MenuRow>(&current_row_))
				{
					switch(*main_menu_row)
					{
					case MenuRow::NewGame:
						if(next_game_ == nullptr)
						{
							next_game_ = CreateGameByIndex(0, sound_player_);
						}
						break;

					case MenuRow::ContinueGame:
						if(next_game_ == nullptr)
						{
							next_game_ = CreateGameById(progress_.current_game, sound_player_);
						}
						break;

					case MenuRow::SelectGame:
						current_row_ = SelectGameMenuRow(0);
						break;

					case MenuRow::Quit:
						quit_triggered_ = true;
						break;

					case MenuRow::NumRows:
						assert(false);
						break;
					}
				}
				else if(const auto select_game_row = std::get_if<SelectGameMenuRow>(&current_row_))
				{
					if(next_game_ == nullptr &&
						(uint32_t(*select_game_row) == 0 ||
						(progress_.opened_games_mask & (1 << uint32_t(*select_game_row))) != 0))
					{
						next_game_ = CreateGameById(*select_game_row, sound_player_);
					}
				}
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_UP)
			{
				if(const auto main_menu_row = std::get_if<MenuRow>(&current_row_))
				{
					*main_menu_row =
						MenuRow((uint32_t(*main_menu_row) + uint32_t(MenuRow::NumRows) - 1) % uint32_t(MenuRow::NumRows));
				}
				else if(const auto select_game_row = std::get_if<SelectGameMenuRow>(&current_row_))
				{
					*select_game_row =
						SelectGameMenuRow((uint32_t(*select_game_row) + uint32_t(SelectGameMenuRow::NumGames) - 1) % uint32_t(SelectGameMenuRow::NumGames));
				}
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_DOWN)
			{
				if(const auto main_menu_row = std::get_if<MenuRow>(&current_row_))
				{
					*main_menu_row = MenuRow((uint32_t(*main_menu_row) + 1) % uint32_t(MenuRow::NumRows));
				}
				else if(const auto select_game_row = std::get_if<SelectGameMenuRow>(&current_row_))
				{
					*select_game_row = SelectGameMenuRow((uint32_t(*select_game_row) + 1) % uint32_t(SelectGameMenuRow::NumGames));
				}
			}
			if(event.key.keysym.scancode >= SDL_SCANCODE_F1 && event.key.keysym.scancode <= SDL_SCANCODE_F12)
			{
				// TODO - consider this cheat.
				if(next_game_ == nullptr)
				{
					next_game_ = CreateGameByIndex(event.key.keysym.scancode - SDL_SCANCODE_F1, sound_player_);
				}
			}
		}
	}

	++tick_;
}

void GameMainMenu::Draw(const FrameBuffer frame_buffer) const
{
	DrawSprite(frame_buffer, Sprites::kloster_unser_lieben_frauen_magdeburg, 0, 0);

	const SpriteBMP game_name_sprite(Sprites::game_name);
	DrawSpriteWithAlpha(frame_buffer, game_name_sprite, 0, (frame_buffer.width - game_name_sprite.GetWidth()) / 2, 4);

	const uint32_t offset_x = 112;
	const uint32_t offset_y = 96;
	const uint32_t row_step = 2 * g_glyph_height;
	const uint32_t cursor_offset = 3 * g_glyph_width;

	const Color32 texts_color = g_cga_palette[11];
	const Color32 cursor_color = g_cga_palette[10];
	const bool draw_cursor = tick_ / 32 % 2 != 0;

	if(const auto main_menu_row = std::get_if<MenuRow>(&current_row_))
	{
		const char* const texts[] = {"New game", "Continue game", "Select game", "Quit"};
		for(uint32_t i = 0; i < uint32_t(std::size(texts)); ++i)
		{
			DrawText(
				frame_buffer,
				texts_color,
				offset_x,
				offset_y + row_step * i,
				texts[i]);
		}

		if(draw_cursor)
		{
			DrawText(
				frame_buffer,
				cursor_color,
				offset_x - cursor_offset,
				offset_y + row_step * uint32_t(*main_menu_row),
				"=>");
		}
	}
	if(const auto select_game_row = std::get_if<SelectGameMenuRow>(&current_row_))
	{
		const char* const game_names[]
		{
			"Arkanoid",
			"Tetris",
			"Snake",
			"Pacman",
		};
		for(uint32_t i = 0; i < uint32_t(SelectGameMenuRow::NumGames); ++i)
		{
			DrawText(
				frame_buffer,
				texts_color,
				offset_x,
				offset_y + row_step * uint32_t(i),
				(i == 0 || ((1 << i) & progress_.opened_games_mask) != 0)
					? game_names[i]
					: "????????????????");
		}

		if(draw_cursor)
		{
			DrawText(
				frame_buffer,
				cursor_color,
				offset_x - cursor_offset,
				offset_y + row_step * uint32_t(*select_game_row),
				"=>");
		}
	}
}

GameInterfacePtr GameMainMenu::AskForNextGameTransition()
{
	return std::move(next_game_);
}

bool GameMainMenu::AskForQuit()
{
	return quit_triggered_;
}
