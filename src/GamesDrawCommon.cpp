#include "GamesDrawCommon.hpp"
#include "Draw.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"

namespace
{

const SpriteBMP g_tetris_blocks[g_tetris_num_piece_types]
{
	Sprites::tetris_block_4,
	Sprites::tetris_block_7,
	Sprites::tetris_block_5,
	Sprites::tetris_block_1,
	Sprites::tetris_block_2,
	Sprites::tetris_block_6,
	Sprites::tetris_block_3,
};

} // namespace

uint32_t GetTetrisFieldOffsetX(const FrameBuffer frame_buffer)
{
	return (frame_buffer.width - g_tetris_field_width * g_tetris_blocks[0].GetWidth()) / 2;
}

uint32_t GetTetrisFieldOffsetY(const FrameBuffer frame_buffer)
{
	(void)frame_buffer;
	return g_tetris_blocks[0].GetHeight();
}

void DrawTetrisFieldBorder(const FrameBuffer frame_buffer, const bool curt_upper_segments)
{
	const SpriteBMP border_sprite(Sprites::tetris_block_8);

	const uint32_t block_width  = border_sprite.GetWidth ();
	const uint32_t block_height = border_sprite.GetHeight();

	const uint32_t field_offset_x = GetTetrisFieldOffsetX(frame_buffer);
	const uint32_t field_offset_y = GetTetrisFieldOffsetY(frame_buffer);;

	for(uint32_t x = 0; x < g_tetris_field_width; ++x)
	{
		DrawSprite(
			frame_buffer,
			border_sprite,
			field_offset_x + x * block_width,
			field_offset_y + g_tetris_field_height * block_height);
	}
	for(uint32_t y = curt_upper_segments ? 2 : 0; y < g_tetris_field_height + 2; ++y)
	{
		DrawSprite(
			frame_buffer,
			border_sprite,
			field_offset_x - 1 * block_width,
			field_offset_y + y * block_height - block_height);
		DrawSprite(
			frame_buffer,
			border_sprite,
			field_offset_x + g_tetris_field_width * block_width,
			field_offset_y + y * block_height - block_height);
	}
}


void DrawTetrisField(
	const FrameBuffer frame_buffer,
	const uint32_t offset_x,
	const uint32_t offset_y,
	const TetrisBlock* const blocks,
	const uint32_t field_width,
	const uint32_t field_height)
{
	const uint32_t block_width  = g_tetris_blocks[0].GetWidth ();
	const uint32_t block_height = g_tetris_blocks[0].GetHeight();

	for(uint32_t y = 0; y < field_height; ++y)
	for(uint32_t x = 0; x < field_width ; ++x)
	{
		const TetrisBlock block = blocks[x + y * field_width];
		if(block == TetrisBlock::Empty)
		{
			continue;
		}

		DrawSpriteWithAlpha(
			frame_buffer,
			g_tetris_blocks[uint32_t(block) - 1],
			0,
			offset_x + x * block_width,
			offset_y + y * block_height);
	}
}
