#include "GameBattleCity.hpp"
#include "BattleCityLevels.hpp"
#include "GameMainMenu.hpp"
#include "Draw.hpp"
#include "SpriteBMP.hpp"
#include "Sprites.hpp"
#include <cassert>

namespace
{

const fixed16_t g_player_speed = g_fixed16_one * 4 / GameInterface::c_update_frequency;
const fixed16_t g_player_half_size = g_fixed16_one;

} // namespace

GameBattleCity::GameBattleCity(SoundPlayer& sound_player)
	: sound_player_(sound_player)
{
	FillField(battle_city_level_0);

	Player player;
	player.position = {IntToFixed16(int32_t(c_field_width / 2 - 4)), IntToFixed16(int32_t(c_field_height - 1))};
	player.direction = GridDirection::YMinus;

	player_ = player;
}

void GameBattleCity::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
	}

	if(player_ != std::nullopt)
	{
		ProcessPlayerInput(keyboard_state);
	}
}

void GameBattleCity::Draw(const FrameBuffer frame_buffer) const
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_width  = c_field_width  * c_block_size;
	const uint32_t field_height = c_field_height * c_block_size;

	const uint32_t field_offset_x = c_block_size * 2;
	const uint32_t field_offset_y = (frame_buffer.height - field_height) / 2;

	const uint32_t field_x_end = field_offset_x + field_width ;
	const uint32_t field_y_end = field_offset_y + field_height;

	const Color32 border_color = g_cga_palette[8];
	FillRect(frame_buffer, border_color, 0, 0, frame_buffer.width, field_offset_y);
	FillRect(frame_buffer, border_color, 0, field_offset_y + field_height, frame_buffer.width, frame_buffer.height - field_y_end);
	FillRect(frame_buffer, border_color, 0, field_offset_y, field_offset_x, field_height);
	FillRect(frame_buffer, border_color, field_x_end, field_offset_y, frame_buffer.width - field_x_end, field_height);

	const SpriteBMP block_sprites[]
	{
		Sprites::battle_city_block_bricks,
		Sprites::battle_city_block_bricks,
		Sprites::battle_city_block_concrete,
		Sprites::battle_city_block_foliage,
		Sprites::battle_city_block_water,
	};

	// Draw field except foliage.
	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		const Block& block = field_[x + y * c_field_width];
		if(block.type == BlockType::Empty || block.type == BlockType::Foliage || block.destruction_mask == 0)
		{
			continue;
		}

		DrawSpriteWithAlpha(
			frame_buffer,
			block_sprites[size_t(block.type)],
			0,
			field_offset_x + x * c_block_size,
			field_offset_y + y * c_block_size);
	}

	if(player_ != std::nullopt)
	{
		auto func = DrawSpriteWithAlpha;
		switch(player_->direction)
		{
		case GridDirection::XMinus:
			func = DrawSpriteWithAlphaRotate270;
			break;
		case GridDirection::XPlus:
			func = DrawSpriteWithAlphaRotate90;
			break;
		case GridDirection::YMinus:
			func = DrawSpriteWithAlpha;
			break;
		case GridDirection::YPlus:
			func = DrawSpriteWithAlphaRotate180;
			break;
		}

		const uint32_t x = uint32_t(Fixed16FloorToInt(player_->position[0] * int32_t(c_block_size)));
		const uint32_t y = uint32_t(Fixed16FloorToInt(player_->position[1] * int32_t(c_block_size)));

		const bool use_a = ((x ^ y) & 1) != 0;

		const SpriteBMP sprite(use_a ? Sprites::battle_city_player_0_a : Sprites::battle_city_player_0_b);
		func(
			frame_buffer,
			sprite,
			0,
			field_offset_x + x - sprite.GetWidth () / 2,
			field_offset_y + y - sprite.GetHeight() / 2);
	}

	// Draw foliage after player and enemies.
	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		const Block& block = field_[x + y * c_field_width];
		if(block.type != BlockType::Foliage || block.destruction_mask == 0)
		{
			continue;
		}

		DrawSpriteWithAlpha(
			frame_buffer,
			block_sprites[size_t(BlockType::Foliage)],
			0,
			field_offset_x + x * c_block_size,
			field_offset_y + y * c_block_size);
	}

	// Base.
	DrawSpriteWithAlpha(
		frame_buffer,
		Sprites::battle_city_eagle,
		0,
		field_offset_x + c_block_size * (c_field_width / 2 - 1),
		field_offset_y + c_block_size * (c_field_height - 2));
}

GameInterfacePtr GameBattleCity::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameBattleCity::ProcessPlayerInput(const std::vector<bool>& keyboard_state)
{
	fixed16vec2_t new_position = player_->position;

	if(keyboard_state.size() > size_t(SDL_SCANCODE_LEFT) && keyboard_state[size_t(SDL_SCANCODE_LEFT)])
	{
		player_->direction = GridDirection::XMinus;
		new_position[0] -= g_player_speed;
		new_position[1] = IntToFixed16(Fixed16RoundToInt(player_->position[1]));
	}
	else if(keyboard_state.size() > size_t(SDL_SCANCODE_RIGHT) && keyboard_state[size_t(SDL_SCANCODE_RIGHT)])
	{
		player_->direction = GridDirection::XPlus;
		new_position[0] += g_player_speed;
		new_position[1] = IntToFixed16(Fixed16RoundToInt(player_->position[1]));
	}
	else if(keyboard_state.size() > size_t(SDL_SCANCODE_UP) && keyboard_state[size_t(SDL_SCANCODE_UP)])
	{
		player_->direction = GridDirection::YMinus;
		new_position[1] -= g_player_speed;
		new_position[0] = IntToFixed16(Fixed16RoundToInt(player_->position[0]));
	}
	else if(keyboard_state.size() > size_t(SDL_SCANCODE_DOWN) && keyboard_state[size_t(SDL_SCANCODE_DOWN)])
	{
		player_->direction = GridDirection::YPlus;
		new_position[1] += g_player_speed;
		new_position[0] = IntToFixed16(Fixed16RoundToInt(player_->position[0]));
	}

	if(CanMove(
		{new_position[0] - g_player_half_size, new_position[1] - g_player_half_size},
		{new_position[0] + g_player_half_size, new_position[1] + g_player_half_size}))
	{
		player_->position = new_position;
	}

	// Correct player position - do not allow to move outside field borders.
	const fixed16_t border_x_start = 0 + g_player_half_size;
	const fixed16_t border_x_end   = IntToFixed16(int32_t(c_field_width )) - g_player_half_size;
	const fixed16_t border_y_start = 0 + g_player_half_size;
	const fixed16_t border_y_end   = IntToFixed16(int32_t(c_field_height)) - g_player_half_size;

	if(player_->position[0] < border_x_start)
	{
		player_->position[0] = border_x_start;
	}
	if(player_->position[0] > border_x_end)
	{
		player_->position[0] = border_x_end;
	}
	if(player_->position[1] < border_y_start)
	{
		player_->position[1] = border_y_start;
	}
	if(player_->position[1] > border_y_end)
	{
		player_->position[1] = border_y_end;
	}
}

bool GameBattleCity::CanMove(const fixed16vec2_t& min, const fixed16vec2_t& max) const
{
	const int32_t min_x = Fixed16FloorToInt(min[0]);
	const int32_t min_y = Fixed16FloorToInt(min[1]);
	const int32_t max_x = Fixed16CeilToInt(max[0]);
	const int32_t max_y = Fixed16CeilToInt(max[1]);

	for(int32_t y = std::max(0, min_y); y < std::min(max_y, int32_t(c_field_height)); ++y)
	for(int32_t x = std::max(0, min_x); x < std::min(max_x, int32_t(c_field_width )); ++x)
	{
		const Block& block = field_[uint32_t(x) + uint32_t(y) * c_field_width];
		if(block.type == BlockType::Empty || block.type == BlockType::Foliage)
		{
			continue;
		}
		if(block.destruction_mask == 0)
		{
			continue;
		}

		// Solid block - can't move.
		return false;
	}

	return true;
}

void GameBattleCity::FillField(const char* field_data)
{
	for(uint32_t y = 0; y < c_field_height; ++y)
	{
		for(uint32_t x = 0; x < c_field_width; ++x, ++field_data)
		{
			Block& block = field_[x + y * c_field_width];
			block.type = GetBlockTypeForLevelDataByte(*field_data);

			if(block.type != BlockType::Empty)
			{
				block.destruction_mask = 0xF;
			}
		}
		assert(*field_data == '\n');
		++field_data;
	}
}

GameBattleCity::BlockType GameBattleCity::GetBlockTypeForLevelDataByte(const char b)
{
	switch(b)
	{
	case '=': return BlockType::Bricks;
	case '#': return BlockType::Concrete;
	case '%': return BlockType::Foliage;
	case '~': return BlockType::Water;
	default: return BlockType::Empty;
	};
}
