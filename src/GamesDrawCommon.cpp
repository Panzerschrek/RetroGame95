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

const SpriteBMP g_pacman_ghost_sprites[4][4]
{
	{
		Sprites::pacman_ghost_0_right,
		Sprites::pacman_ghost_0_left ,
		Sprites::pacman_ghost_0_down ,
		Sprites::pacman_ghost_0_up   ,
	},
	{
		Sprites::pacman_ghost_1_right,
		Sprites::pacman_ghost_1_left ,
		Sprites::pacman_ghost_1_down ,
		Sprites::pacman_ghost_1_up   ,
	},
	{
		Sprites::pacman_ghost_2_right,
		Sprites::pacman_ghost_2_left ,
		Sprites::pacman_ghost_2_down ,
		Sprites::pacman_ghost_2_up   ,
	},
	{
		Sprites::pacman_ghost_3_right,
		Sprites::pacman_ghost_3_left ,
		Sprites::pacman_ghost_3_down ,
		Sprites::pacman_ghost_3_up   ,
	},
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
	const uint32_t texts_offset_x = frame_buffer.width - 2 * g_glyph_width;
	const uint32_t texts_offset_y = 32;

	char text[64];

	DrawText(
		frame_buffer,
		g_cga_palette[9],
		texts_offset_x - g_glyph_width * uint32_t(UTF8StringLen(Strings::arkanoid_round)),
		texts_offset_y,
		Strings::arkanoid_round);

	NumToString(text, sizeof(text), level, 5);
	DrawText(
		frame_buffer,
		g_color_white,
		texts_offset_x - g_glyph_width * uint32_t(UTF8StringLen(text)),
		texts_offset_y + g_glyph_height * 2,
		text);

	DrawText(
		frame_buffer,
		g_cga_palette[9],
		texts_offset_x - g_glyph_width * uint32_t(UTF8StringLen(Strings::arkanoid_score)),
		texts_offset_y + g_glyph_height * 8,
		Strings::arkanoid_score);

	NumToString(text, sizeof(text), score, 5);
	DrawText(
		frame_buffer,
		g_color_white,
		texts_offset_x - g_glyph_width * uint32_t(UTF8StringLen(text)),
		texts_offset_y + g_glyph_height * 10,
		text);
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

	const uint32_t texts_offset_x = field_offset_x - g_glyph_width * 12;
	const uint32_t texts_offset_y = field_offset_y + block_height * g_tetris_field_height - g_glyph_height * 3;

	char text[64];
	DrawText(frame_buffer, g_cga_palette[14], texts_offset_x, texts_offset_y, Strings::tetris_level);
	NumToString(text, sizeof(text), level, 4);
	DrawText(frame_buffer, g_color_white, texts_offset_x + g_glyph_width * 6, texts_offset_y, text);

	DrawText(frame_buffer, g_cga_palette[14], texts_offset_x, texts_offset_y + g_glyph_height * 2, Strings::tetris_score);
	NumToString(text, sizeof(text), score, 4);
	DrawText(frame_buffer, g_color_white, texts_offset_x + g_glyph_width * 6, texts_offset_y + g_glyph_height * 2, text);
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
	const uint32_t lives,
	const uint32_t level,
	const uint32_t score)
{
	char text[64];

	const char* const stats_names[]
		{Strings::snake_length, Strings::snake_lives, Strings::snake_level, Strings::snake_score};
	const uint32_t stats[]{length, lives, level, score};
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

void DrawPacmanField(
	const FrameBuffer frame_buffer,
	const char* const field_data,
	const uint32_t field_width,
	const uint32_t field_height,
	const uint32_t x_start,
	const uint32_t y_start,
	const uint32_t x_end,
	const uint32_t y_end)
{
	const char c_wall_symbol = '#';
	const Color32 c_wall_color = g_cga_palette[1];
	for(uint32_t y = y_start; y < y_end; ++y)
	{
		const char* const line = field_data + y * field_width;
		const char* const line_y_minus = field_data + (std::max(1u, y) - 1) * field_width;
		const char* const line_y_plus  = field_data + (std::min(field_height - 2, y) + 1) * field_width;

		for(uint32_t x = x_start; x < x_end ; ++x)
		{
			const char block = line[x];
			if(block != c_wall_symbol)
			{
				continue;
			}

			const uint32_t block_x = x * g_pacman_block_size;
			const uint32_t block_y = y * g_pacman_block_size;
			const auto set_pixel = [&](const uint32_t dx, const uint32_t dy)
			{
				frame_buffer.data[ (block_x + dx) + (block_y + dy) * frame_buffer.width] = c_wall_color;
			};

			const uint32_t x_minus_one_clamped = std::max(x, 1u) - 1;
			const uint32_t x_plus_one_clamped = std::min(x, field_width - 2) + 1;

			const bool block_y_minus = line_y_minus[x] == c_wall_symbol;
			const bool block_y_plus  = line_y_plus [x] == c_wall_symbol;
			const bool block_x_minus = line[x_minus_one_clamped] == c_wall_symbol;
			const bool block_x_plus  = line[x_plus_one_clamped ] == c_wall_symbol;
			const bool block_x_minus_y_minus = line_y_minus[x_minus_one_clamped] == c_wall_symbol;
			const bool block_x_minus_y_plus  = line_y_plus [x_minus_one_clamped] == c_wall_symbol;
			const bool block_x_plus_y_minus  = line_y_minus[x_plus_one_clamped ] == c_wall_symbol;
			const bool block_x_plus_y_plus   = line_y_plus [x_plus_one_clamped ] == c_wall_symbol;

			// Sides.
			if((block_x_plus && block_x_minus) || (!block_y_minus && !block_y_plus))
			{
				if(!block_y_minus)
				{
					for(uint32_t dx = 0; dx < g_pacman_block_size; ++dx)
						set_pixel(dx, 4);
				}
				if(!block_y_plus )
				{
					for(uint32_t dx = 0; dx < g_pacman_block_size; ++dx)
						set_pixel(dx, 3);
				}
			}
			if(block_x_plus && block_x_minus)
			{
				if(!block_y_minus && (!block_x_plus_y_plus   || !block_x_minus_y_plus ))
				{
					for(uint32_t dx = 0; dx < g_pacman_block_size; ++dx)
						set_pixel(dx, 3);
				}
				if(!block_y_plus  && (!block_x_plus_y_minus  || !block_x_minus_y_minus))
				{
					for(uint32_t dx = 0; dx < g_pacman_block_size; ++dx)
						set_pixel(dx, 4);
				}
			}
			if((block_y_minus && block_y_plus) || (!block_x_minus && !block_x_plus))
			{
				if(!block_x_minus)
				{
					for(uint32_t dy = 0; dy < g_pacman_block_size; ++dy)
						set_pixel(4, dy);
				}
				if(!block_x_plus)
				{
					for(uint32_t dy = 0; dy < g_pacman_block_size; ++dy)
						set_pixel(3, dy);
				}
			}
			if(block_y_plus && block_y_minus)
			{
				if(!block_x_minus && (!block_x_plus_y_minus  || !block_x_plus_y_plus  ))
				{
					for(uint32_t dy = 0; dy < g_pacman_block_size; ++dy)
						set_pixel(3, dy);
				}
				if(!block_x_plus && (!block_x_minus_y_minus  || !block_x_minus_y_plus ))
				{
					for(uint32_t dy = 0; dy < g_pacman_block_size; ++dy)
						set_pixel(4, dy);
				}
			}

			// Outer corners.
			if(!block_x_minus && !block_y_minus && block_x_plus && block_y_plus)
			{
				set_pixel(4, 6);
				set_pixel(4, 7);
				set_pixel(6, 4);
				set_pixel(7, 4);
				set_pixel(5, 5);
			}
			if(!block_x_plus && !block_y_minus && block_x_minus && block_y_plus)
			{
				set_pixel(3, 6);
				set_pixel(3, 7);
				set_pixel(0, 4);
				set_pixel(1, 4);
				set_pixel(2, 5);
			}
			if(!block_x_minus && !block_y_plus  && block_x_plus  && block_y_minus)
			{
				set_pixel(4, 0);
				set_pixel(4, 1);
				set_pixel(6, 3);
				set_pixel(7, 3);
				set_pixel(5, 2);
			}
			if(!block_x_plus && !block_y_plus  && block_x_minus && block_y_minus)
			{
				set_pixel(3, 0);
				set_pixel(3, 1);
				set_pixel(0, 3);
				set_pixel(1, 3);
				set_pixel(2, 2);
			}

			// Inner corners.
			if(block_x_minus && block_y_minus && !block_x_minus_y_minus)
			{
				set_pixel(4, 0);
				set_pixel(4, 1);
				set_pixel(4, 2);
				set_pixel(0, 4);
				set_pixel(1, 4);
				set_pixel(2, 4);
				set_pixel(3, 3);
			}
			if(block_x_minus && block_y_plus && !block_x_minus_y_plus )
			{
				set_pixel(4, 5);
				set_pixel(4, 6);
				set_pixel(4, 7);
				set_pixel(0, 3);
				set_pixel(1, 3);
				set_pixel(2, 3);
				set_pixel(3, 4);
			}
			if(block_x_plus && block_y_minus && !block_x_plus_y_minus  )
			{
				set_pixel(3, 0);
				set_pixel(3, 1);
				set_pixel(3, 2);
				set_pixel(5, 4);
				set_pixel(6, 4);
				set_pixel(7, 4);
				set_pixel(4, 3);
			}
			if(block_x_plus && block_y_plus  && !block_x_plus_y_plus  )
			{
				set_pixel(3, 5);
				set_pixel(3, 6);
				set_pixel(3, 7);
				set_pixel(5, 3);
				set_pixel(6, 3);
				set_pixel(7, 3);
				set_pixel(4, 4);
			}
		} // for x
	} // for y
}

SpriteBMP GetPacmanGhostSprite(const PacmanGhostType ghost_type, const GridDirection ghost_direction)
{
	return g_pacman_ghost_sprites[size_t(ghost_type)][size_t(ghost_direction)];
}
