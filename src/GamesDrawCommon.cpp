#include "GamesDrawCommon.hpp"
#include "Draw.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"
#include "String.hpp"
#include "Strings.hpp"
#include <cassert>

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

void DrawArkanoidFieldBorder(const FrameBuffer frame_buffer, const bool draw_exit)
{
	const SpriteBMP sprites_trim_top[]
	{
		Sprites::arkanoid_trim_corner_top_left,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_1,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_1,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_1,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_1,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_segment_top_0,
		Sprites::arkanoid_trim_corner_top_right,
	};

	uint32_t trim_top_x = g_arkanoid_field_offset_x - 10;
	for(const SpriteBMP& sprite : sprites_trim_top)
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			trim_top_x,
			g_arkanoid_field_offset_y - 10);

		trim_top_x += sprite.GetWidth();
	}

	const SpriteBMP sprites_trim_left[]
	{
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_1,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_1,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_1,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_1,
	};

	uint32_t trim_side_y = g_arkanoid_field_offset_y;
	const uint32_t side_trim_offset_x = g_arkanoid_field_offset_x - 10;
	for(const SpriteBMP& sprite : sprites_trim_left)
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			side_trim_offset_x,
			trim_side_y);

		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			side_trim_offset_x + g_arkanoid_block_width * g_arkanoid_field_width + sprite.GetWidth(),
			trim_side_y);

		trim_side_y += sprite.GetHeight();
	}

	if(draw_exit)
	{
		const SpriteBMP sprite(Sprites::arkanoid_level_exit_gate);
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			side_trim_offset_x + g_arkanoid_block_width * g_arkanoid_field_width + sprite.GetWidth(),
			trim_side_y);
	}

	// Draw two lover sprites of side trimming, including level exit.
	for(size_t i = 0; i < 2; ++i)
	{
		const SpriteBMP sprite(Sprites::arkanoid_trim_segment_side_0);
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			side_trim_offset_x,
			trim_side_y);

		if(!draw_exit)
		{
			DrawSpriteWithAlpha(
				frame_buffer,
				sprite,
				0,
				side_trim_offset_x + g_arkanoid_block_width * g_arkanoid_field_width + sprite.GetWidth(),
				trim_side_y);
		}

		trim_side_y += sprite.GetHeight();
	}
}

void DrawArkanoidField(const FrameBuffer frame_buffer, const ArkanoidBlock* const field)
{
	DrawArkanoidField(frame_buffer, field, 0, g_arkanoid_field_width);
}

void DrawArkanoidField(
	FrameBuffer frame_buffer,
	const ArkanoidBlock* const field,
	const uint32_t start_column,
	const uint32_t end_column)
{
	assert(start_column <= end_column);
	assert(end_column <= g_arkanoid_field_width);

	const SpriteBMP block_sprites[]
	{
		Sprites::arkanoid_block_1,
		Sprites::arkanoid_block_2,
		Sprites::arkanoid_block_3,
		Sprites::arkanoid_block_4,
		Sprites::arkanoid_block_5,
		Sprites::arkanoid_block_6,
		Sprites::arkanoid_block_7,
		Sprites::arkanoid_block_8,
		Sprites::arkanoid_block_9,
		Sprites::arkanoid_block_10,
		Sprites::arkanoid_block_11,
		Sprites::arkanoid_block_12,
		Sprites::arkanoid_block_13,
		Sprites::arkanoid_block_14,
		Sprites::arkanoid_block_15,
		Sprites::arkanoid_block_concrete,
		Sprites::arkanoid_block_14_15,
	};

	for(uint32_t y = 0; y < g_arkanoid_field_height; ++y)
	{
		for(uint32_t x = start_column; x < end_column; ++x)
		{
			const ArkanoidBlock& block = field[x + y * g_arkanoid_field_width];
			if(block.type == ArkanoidBlockType::Empty)
			{
				continue;
			}
			DrawSpriteWithAlpha(
				frame_buffer,
				block_sprites[uint32_t(block.type) - 1],
				0,
				g_arkanoid_field_offset_x + x * g_arkanoid_block_width,
				g_arkanoid_field_offset_y + y * g_arkanoid_block_height);
		}
	}
}

void DrawArkanoidLevelStartSplash(const FrameBuffer frame_buffer, const uint32_t level)
{
	DrawTextCentered(
		frame_buffer,
		g_cga_palette[9],
		g_arkanoid_field_offset_x + g_arkanoid_block_width  * g_arkanoid_field_width  / 2,
		g_arkanoid_field_offset_y + g_arkanoid_block_height * (g_arkanoid_field_height - 6),
		Strings::arkanoid_round);

	char text[64];
	NumToString(text, sizeof(text), level, 0);
	DrawTextCentered(
		frame_buffer,
		g_color_white,
		g_arkanoid_field_offset_x + g_arkanoid_block_width  * g_arkanoid_field_width  / 2,
		g_arkanoid_field_offset_y + g_arkanoid_block_height * (g_arkanoid_field_height - 6) + g_glyph_height * 2,
		text);
}

void DrawArakoindStats(const FrameBuffer frame_buffer, const uint32_t level, const uint32_t score)
{
	const uint32_t texts_offset_x = 264;
	const uint32_t texts_offset_y = 32;

	char text[64];

	DrawText(frame_buffer, g_cga_palette[9], texts_offset_x, texts_offset_y, Strings::arkanoid_round);

	NumToString(text, sizeof(text), level, 5);
	DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y + g_glyph_height * 2, text);

	DrawText(frame_buffer, g_cga_palette[9], texts_offset_x, texts_offset_y + 64, Strings::arkanoid_score);

	NumToString(text, sizeof(text), score, 5);
	DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y + 64 + g_glyph_height * 2, text);
}

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

void DrawTetrisNextPiece(const FrameBuffer frame_buffer, const TetrisBlock next_piece)
{
	if(next_piece == TetrisBlock::Empty)
	{
		return;
	}

	const uint32_t block_width  = g_tetris_blocks[0].GetWidth ();
	const uint32_t block_height = g_tetris_blocks[0].GetHeight();

	const uint32_t field_offset_x = GetTetrisFieldOffsetX(frame_buffer);
	const uint32_t field_offset_y = GetTetrisFieldOffsetY(frame_buffer);
	const uint32_t next_piece_offset_x = field_offset_x + block_width * (g_tetris_field_width - 2);
	const uint32_t next_piece_offset_y = field_offset_y + 7 * block_height;

	const auto next_piece_index = uint32_t(next_piece) - uint32_t(TetrisBlock::I);

	const uint8_t pieces_colors[g_tetris_num_piece_types]{ 4, 7, 5, 1, 2, 6, 3, };
	DrawText(
		frame_buffer,
		g_cga_palette[pieces_colors[uint32_t(next_piece_index)]],
		next_piece_offset_x + block_width * 4,
		next_piece_offset_y - block_height * 6,
		Strings::tetris_next);

	for(const auto& piece_block : g_tetris_pieces_blocks[next_piece_index])
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			g_tetris_blocks[next_piece_index],
			0,
			next_piece_offset_x + uint32_t(piece_block[0]) * block_width,
			next_piece_offset_y + uint32_t(piece_block[1]) * block_height);
	}
}

void DrawTetrisStats(const FrameBuffer frame_buffer, const uint32_t level, const uint32_t score)
{
	const uint32_t block_height = g_tetris_blocks[0].GetHeight();

	const uint32_t field_offset_x = GetTetrisFieldOffsetX(frame_buffer);
	const uint32_t field_offset_y = GetTetrisFieldOffsetY(frame_buffer);

	const uint32_t texts_offset_x = field_offset_x - g_glyph_width * 13;
	const uint32_t texts_offset_y = field_offset_y + block_height * g_tetris_field_height - g_glyph_height * 3;

	char text[64];
	DrawText(frame_buffer, g_cga_palette[14], texts_offset_x, texts_offset_y, Strings::tetris_level);
	NumToString(text, sizeof(text), level, 3);
	DrawText(frame_buffer, g_color_white, texts_offset_x + g_glyph_width * 7, texts_offset_y, text);

	DrawText(frame_buffer, g_cga_palette[14], texts_offset_x, texts_offset_y + g_glyph_height * 2, Strings::tetris_score);
	NumToString(text, sizeof(text), score, 3);
	DrawText(frame_buffer, g_color_white, texts_offset_x + g_glyph_width * 7, texts_offset_y + g_glyph_height * 2, text);
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

void DrawSnakeStats(
	const FrameBuffer frame_buffer,
	const uint32_t length,
	const uint32_t lifes,
	const uint32_t level,
	const uint32_t score)
{
	char text[64];

	const char* const stats_names[]
		{Strings::snake_length, Strings::snake_lives, Strings::snake_level, Strings::snake_score};
	const uint32_t stats[]{length, lifes, level, score};
	const uint8_t stats_colors[]{6, 4, 1, 2};
	for(uint32_t i = 0; i < 4; ++i)
	{
		const uint32_t x = (i + 1) * g_glyph_width * 9;
		const uint32_t len = uint32_t(UTF8StringLen(stats_names[i]));

		DrawText(
			frame_buffer,
			g_cga_palette[stats_colors[i]],
			x - g_glyph_width * len,
			frame_buffer.height - g_glyph_height * 2 - 3,
			stats_names[i]);

		NumToString(text, sizeof(text), stats[i], 5);
		DrawText(
			frame_buffer,
			g_color_white,
			x - g_glyph_width * 5,
			frame_buffer.height - g_glyph_height - 1,
			text);
	}
}
