#include "GameTetris.hpp"
#include "Draw.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"

namespace
{

const constexpr uint32_t g_num_piece_types = 7;

const  std::array<std::array<std::array<int32_t, 2>, 4>, g_num_piece_types> g_pieces_blocks =
{{
	// TODO - fix this.
	{{ { 4, -3}, {5, -2}, {4, -1}, {5, 0} }},
	{{ { 4, -1}, {5, -1}, {4, 0}, {5, 0} }},
	{{ { 4, -1}, {5, -1}, {4, 0}, {5, 0} }},
	{{ { 4, -1}, {5, -1}, {4, 0}, {5, 0} }},
	{{ { 4, -1}, {5, -1}, {4, 0}, {5, 0} }},
	{{ { 4, -1}, {5, -1}, {4, 0}, {5, 0} }},
	{{ { 4, -1}, {5, -1}, {4, 0}, {5, 0} }},
}};

} // namespace

GameTetris::GameTetris()
	: rand_() // TODO - use random seed.
{
	for (Block& block : field_)
	{
		block = Block::Empty;
	}

	active_piece_ = SpawnActivePiece();
}

void GameTetris::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	++num_ticks_;
	if(num_ticks_ % speed_ == 0)
	{
		TickInternal();
	}

	(void) events;
	(void) keyboard_state;
	// TODO
}

void GameTetris::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_offset_x = 16;
	const uint32_t field_offset_y = 8;

	const SpriteBMP sprites[]
	{
		Sprites::tetris_block_1,
		Sprites::tetris_block_2,
		Sprites::tetris_block_3,
		Sprites::tetris_block_4,
		Sprites::tetris_block_5,
		Sprites::tetris_block_6,
		Sprites::tetris_block_7,
	};

	for(uint32_t y = 0; y < 20; ++y)
	{
		for(uint32_t x = 0; x < 10; ++x)
		{
			const Block block = field_[x + y * c_field_width];
			if(block == Block::Empty)
			{
				continue;
			}

			const SpriteBMP& sprite = sprites[uint32_t(block) - 1];
			DrawSpriteWithAlphaUnchecked(
				frame_buffer,
				sprite,
				0,
				field_offset_x + x * sprite.GetWidth(),
				field_offset_y + y * sprite.GetHeight());
		}
	}

	if(active_piece_ != std::nullopt)
	{
		for(const auto& piece_block : active_piece_->blocks)
		{
			if(piece_block[0] >= 0 && piece_block[0] < int32_t(c_field_width) &&
				piece_block[1] >= 0 && piece_block[1] < int32_t(c_field_height))
			{
				const SpriteBMP& sprite = sprites[uint32_t(active_piece_->type) - 1];
				DrawSpriteWithAlphaUnchecked(
					frame_buffer,
					sprite,
					0,
					field_offset_x + uint32_t(piece_block[0]) * sprite.GetWidth(),
					field_offset_y + uint32_t(piece_block[1]) * sprite.GetHeight());
			}
		}
	}
}

GameInterfacePtr GameTetris::AskForNextGameTransition()
{
	// TODO
	return nullptr;
}

void GameTetris::TickInternal()
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

			// TODO - remove filled lines.

			active_piece_ = std::nullopt;
		}
	}
}

GameTetris::ActivePiece GameTetris::SpawnActivePiece()
{
	ActivePiece piece;
	piece.type = GenerateNextPieceType();
	piece.blocks = g_pieces_blocks[uint32_t(piece.type) - uint32_t(Block::I)];
	return piece;
}

GameTetris::Block GameTetris::GenerateNextPieceType()
{
	return Block(uint32_t(Block::I) + rand_.Next() % g_num_piece_types);
}
