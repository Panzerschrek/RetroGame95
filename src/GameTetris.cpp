#include "GameTetris.hpp"
#include "Draw.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"

namespace
{

const constexpr uint32_t g_num_piece_types = 7;

const std::array<std::array<std::array<int32_t, 2>, 4>, g_num_piece_types> g_pieces_blocks =
{{
	{{ { 4, -4}, {4, -3}, {4, -2}, {4, -1} }}, // I
	{{ { 4, -1}, {5, -1}, {5, -2}, {5, -3} }}, // J
	{{ { 5, -1}, {4, -1}, {4, -2}, {4, -3} }}, // L
	{{ { 4, -2}, {5, -2}, {4, -1}, {5, -1} }}, // O
	{{ { 4, -1}, {5, -1}, {5, -2}, {6, -2} }}, // S
	{{ { 4, -1}, {6, -1}, {5, -1}, {5, -2} }}, // T
	{{ { 5, -1}, {6, -1}, {4, -2}, {5, -2} }}, // Z
}};

} // namespace

GameTetris::GameTetris()
	: rand_() // TODO - use random seed.
{
	for (Block& block : field_)
	{
		block = Block::Empty;
	}

	GenerateNextPieceType();
	active_piece_ = SpawnActivePiece();
}

void GameTetris::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	(void) keyboard_state;

	++num_ticks_;

	ManipulatePiece(events);

	if(num_ticks_ % speed_ == 0)
	{
		MovePieceDown();
	}
}

void GameTetris::Draw(const FrameBuffer frame_buffer)
{
	const SpriteBMP sprites[g_num_piece_types]
	{
		Sprites::tetris_block_4,
		Sprites::tetris_block_7,
		Sprites::tetris_block_5,
		Sprites::tetris_block_1,
		Sprites::tetris_block_2,
		Sprites::tetris_block_6,
		Sprites::tetris_block_3,
	};

	const uint32_t block_width = sprites[0].GetWidth();
	const uint32_t block_height = sprites[1].GetHeight();

	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_offset_x = 96;
	const uint32_t field_offset_y = 8;
	const uint32_t next_piece_offset_x = field_offset_x + block_width * (c_field_width - 2);
	const uint32_t next_piece_offset_y = field_offset_y + 5 * block_height;

	DrawHorisontalLine(
		frame_buffer,
		g_color_white,
		field_offset_x - 1,
		field_offset_y - 1,
		block_width * c_field_width + 2);
	DrawHorisontalLine(
		frame_buffer,
		g_color_white,
		field_offset_x - 1,
		field_offset_y + block_height * c_field_height,
		block_width * c_field_width + 2);

	DrawVerticaLine(
		frame_buffer,
		g_color_white,
		field_offset_x - 1,
		field_offset_y - 1,
		block_height * c_field_height + 2);
	DrawVerticaLine(
		frame_buffer,
		g_color_white,
		field_offset_x + block_width * c_field_width,
		field_offset_y - 1,
		block_height * c_field_height + 2);

	for(uint32_t y = 0; y < 20; ++y)
	{
		for(uint32_t x = 0; x < 10; ++x)
		{
			const Block block = field_[x + y * c_field_width];
			if(block == Block::Empty)
			{
				continue;
			}

			DrawSpriteWithAlphaUnchecked(
				frame_buffer,
				sprites[uint32_t(block) - 1],
				0,
				field_offset_x + x * block_width,
				field_offset_y + y * block_height);
		}
	}

	if(active_piece_ != std::nullopt)
	{
		for(const auto& piece_block : active_piece_->blocks)
		{
			if(piece_block[0] >= 0 && piece_block[0] < int32_t(c_field_width) &&
				piece_block[1] >= 0 && piece_block[1] < int32_t(c_field_height))
			{
				DrawSpriteWithAlphaUnchecked(
					frame_buffer,
					sprites[uint32_t(active_piece_->type) - 1],
					0,
					field_offset_x + uint32_t(piece_block[0]) * block_width,
					field_offset_y + uint32_t(piece_block[1]) * block_height);
			}
		}
	}

	const auto next_piece_index = uint32_t(next_piece_type_) - uint32_t(Block::I);
	for(const auto& piece_block : g_pieces_blocks[next_piece_index])
	{
		DrawSpriteWithAlphaUnchecked(
			frame_buffer,
			sprites[next_piece_index],
			0,
			next_piece_offset_x + uint32_t(piece_block[0]) * block_width,
			next_piece_offset_y + uint32_t(piece_block[1]) * block_height);
	}
}

GameInterfacePtr GameTetris::AskForNextGameTransition()
{
	// TODO
	return nullptr;
}

void GameTetris::ManipulatePiece(const std::vector<SDL_Event>& events)
{
	if (active_piece_ == std::nullopt)
	{
		return;
	}

	bool has_move_left = false;
	bool has_move_right = false;
	bool has_move_down = false;
	bool has_rotate = false;
	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN)
		{

			has_move_left |= event.key.keysym.scancode == SDL_SCANCODE_LEFT;
			has_move_right |= event.key.keysym.scancode == SDL_SCANCODE_RIGHT;
			has_move_down |= event.key.keysym.scancode == SDL_SCANCODE_DOWN;
			has_rotate |= event.key.keysym.scancode == SDL_SCANCODE_UP;
		}
	}

	const auto try_side_move_piece =
	[&](const int32_t delta)
	{
		bool can_move = true;
		for(const auto& piece_block : active_piece_->blocks)
		{
			const auto next_x = piece_block[0] + delta;
			const auto next_y = piece_block[1];
			if( next_x < 0 || next_x >= int32_t(c_field_width ) ||
				(next_y >= 0 && next_y < int32_t(c_field_height) && field_[uint32_t(next_x) + uint32_t(next_y) * c_field_width] != Block::Empty))
			{
				can_move = false;
			}
		}

		if(can_move)
		{
			for(auto& piece_block : active_piece_->blocks)
			{
				piece_block[0] += delta;
			}
		}
	};

	if(has_move_left)
	{
		try_side_move_piece(-1);
	}
	if(has_move_right)
	{
		try_side_move_piece(1);
	}

	if(has_move_down)
	{
		bool can_move = true;
		for(const auto& piece_block : active_piece_->blocks)
		{
			const auto next_x = piece_block[0];
			const auto next_y = piece_block[1] + 1;
			if(next_y >= int32_t(c_field_height) ||
				(next_y >= 0 && field_[uint32_t(next_x) + uint32_t(next_y) * c_field_width] != Block::Empty))
			{
				can_move = false;
			}
		}

		if(can_move)
		{
			for(auto& piece_block : active_piece_->blocks)
			{
				piece_block[1] += 1;
			}
		}
	}

	if(has_rotate && active_piece_->type != Block::O)
	{
		const auto center = active_piece_->blocks[2];

		std::array<std::array<int32_t, 2>, 4> blocks_transformed;
		bool can_rotate = true;
		for(size_t i = 0; i < 4; ++i)
		{
			const auto& block = active_piece_->blocks[i];
			const int32_t rel_x = block[0] - center[0];
			const int32_t rel_y = block[1] - center[1];
			const int32_t new_x = center[0] + rel_y;
			const int32_t new_y = center[1] - rel_x;

			blocks_transformed[i] = {new_x, new_y};

			if(new_x < 0 || new_x >= int32_t(c_field_width) ||
				new_y >= int32_t(c_field_height) ||
				(new_y >= 0 && field_[uint32_t(new_x) + uint32_t(new_y) * c_field_width] != Block::Empty))
			{
				can_rotate = false;
			}
		}

		if(can_rotate)
		{
			active_piece_->blocks = blocks_transformed;
		}
	}
}

void GameTetris::MovePieceDown()
{
	if(game_over_)
	{
		return;
	}

	if (active_piece_ == std::nullopt)
	{
		// No active piece - try to spawn new piece.
		ActivePiece next_active_piece = SpawnActivePiece();
		bool next_active_piece_can_be_placed = true;
		for(const auto& piece_block : next_active_piece.blocks)
		{
			if( piece_block[0] >= 0 && piece_block[0] < int32_t(c_field_width ) &&
				piece_block[1] >= 0 && piece_block[1] < int32_t(c_field_height) &&
				field_[uint32_t(piece_block[0]) + uint32_t(piece_block[1]) * c_field_width] != Block::Empty)
			{
				next_active_piece_can_be_placed = false;
			}
		}

		if(next_active_piece_can_be_placed)
		{
			active_piece_ = next_active_piece;
		}
		else
		{
			game_over_ = true;
		}
	}
	else
	{
		bool can_move = true;
		for(const auto& piece_block : active_piece_->blocks)
		{
			if(piece_block[1] == int32_t(c_field_height - 1))
			{
				can_move = false;
			}

			const auto next_x = piece_block[0];
			const auto next_y = piece_block[1] + 1;
			if( next_x >= 0 && next_x < int32_t(c_field_width ) &&
				next_y >= 0 && next_y < int32_t(c_field_height) &&
				field_[uint32_t(next_x) + uint32_t(next_y) * c_field_width] != Block::Empty)
			{
				can_move = false;
			}
		}

		if(can_move)
		{
			for(auto& piece_block : active_piece_->blocks)
			{
				piece_block[1] += 1;
			}
		}
		else
		{
			// Put piece into field.
			for(const auto& piece_block : active_piece_->blocks)
			{
				field_[uint32_t(piece_block[0]) + uint32_t(piece_block[1]) * c_field_width] = active_piece_->type;
			}

			// Remove lines.
			for(uint32_t y = c_field_height -1;;)
			{
				bool line_is_full = true;
				for(uint32_t x = 0; x < c_field_width; ++x)
				{
					line_is_full &= field_[x + y * c_field_width] != Block::Empty;
				}

				if(line_is_full)
				{
					// Remove this line.
					for(uint32_t dst_y = y; ; --dst_y)
					{
						if(dst_y == 0)
						{
							for(uint32_t x = 0; x < c_field_width; ++x)
							{
								field_[x + dst_y * c_field_width] =Block::Empty;
							}
						}
						else
						{
							const uint32_t src_y = dst_y - 1;

							for(uint32_t x = 0; x < c_field_width; ++x)
							{
								field_[x + dst_y * c_field_width] = field_[x + src_y * c_field_width];
								field_[x + src_y * c_field_width] = Block::Empty;
							}
						}

						if(dst_y == 0)
						{
							break;
						}
					} // Shift lines after removal.
				}
				else if (y > 0)
				{
					--y;
				}
				else
				{
					break;
				}
			}

			active_piece_ = std::nullopt;
		}
	}
}

GameTetris::ActivePiece GameTetris::SpawnActivePiece()
{
	ActivePiece piece;
	piece.type = next_piece_type_;
	GenerateNextPieceType();
	piece.blocks = g_pieces_blocks[uint32_t(piece.type) - uint32_t(Block::I)];
	return piece;
}

void GameTetris::GenerateNextPieceType()
{
	++pieces_spawnded_;
	next_piece_type_ = Block(uint32_t(Block::I) + rand_.Next() % g_num_piece_types);
}
