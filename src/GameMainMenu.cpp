#include "GameMainMenu.hpp"
#include "Draw.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"

void GameMainMenu::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	(void) events;
	(void) keyboard_state;
	// TODO
}

void GameMainMenu::Draw(const FrameBuffer frame_buffer)
{
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::gimp_harold), 47, 16);

	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_ghost), 128, 32 );
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_1), 144, 32);
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_food), 144 + 16, 32 + 6);


	const SpriteBMP sprites[]
		{
		Sprites::tetris_block_1,
		Sprites::tetris_block_2,
		Sprites::tetris_block_3,
		Sprites::tetris_block_4,
		Sprites::tetris_block_5,
		Sprites::tetris_block_6,
		Sprites::tetris_block_7
		};
	for(uint32_t y = 0; y < 20; ++y)
	{
		for(uint32_t x = 0; x < 10; ++x)
		{
			const auto& sprite = sprites[ (x / 4 + y/2) % std::size(sprites)];
			DrawSpriteWithAlphaUnchecked(
				frame_buffer,
				sprite,
				0,
				x * sprite.GetWidth(),
				y * sprite.GetHeight());
		}
	}
}

GameInterfacePtr GameMainMenu::AskForNextGameTransition()
{
	// TODO
	return nullptr;
}
