#include "GameTetris.hpp"
#include "Draw.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"

void GameTetris::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	(void) events;
	(void) keyboard_state;
	// TODO
}

void GameTetris::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

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

GameInterfacePtr GameTetris::AskForNextGameTransition()
{
	// TODO
	return nullptr;
}
