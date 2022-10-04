#include "GameBattleCity.hpp"
#include "GameMainMenu.hpp"
#include "Draw.hpp"
#include "SpriteBMP.hpp"
#include "Sprites.hpp"

namespace
{

const fixed16_t g_player_speed = g_fixed16_one * 4 / GameInterface::c_update_frequency;
const fixed16_t g_player_half_size = g_fixed16_one;

} // namespace

GameBattleCity::GameBattleCity(SoundPlayer& sound_player)
	: sound_player_(sound_player)
{
	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		Block& block = field_[x + y * c_field_width];
		block.type = BlockType((y + x) % 5);
		block.destruction_mask = 0xF;
	}

	Player player;
	player.position = {IntToFixed16(int32_t(c_field_width / 2 - 2)), IntToFixed16(int32_t(c_field_height - 1))};
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

	// TODO - use proper sprites.
	const SpriteBMP block_sprites[]
	{
		Sprites::tetris_block_small_4,
		Sprites::tetris_block_small_7,
		Sprites::tetris_block_small_5,
		Sprites::tetris_block_small_1,
		Sprites::tetris_block_small_2,
		Sprites::tetris_block_small_6,
	};

	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		const Block& block = field_[x + y * c_field_width];
		if(block.type == BlockType::Empty || block.destruction_mask == 0)
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
			func = DrawSpriteWithAlphaRotate180;
			break;
		case GridDirection::XPlus:
			func = DrawSpriteWithAlpha;
			break;
		case GridDirection::YMinus:
			func = DrawSpriteWithAlphaRotate270;
			break;
		case GridDirection::YPlus:
			func = DrawSpriteWithAlphaRotate90;
			break;
		}

		// TODO - use proper sprite.
		const SpriteBMP sprite(Sprites::pacman_2);
		func(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(player_->position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(player_->position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}
}

GameInterfacePtr GameBattleCity::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameBattleCity::ProcessPlayerInput(const std::vector<bool>& keyboard_state)
{
	// TODO - do not allow to move towards walls.
	if(keyboard_state.size() > size_t(SDL_SCANCODE_LEFT) && keyboard_state[size_t(SDL_SCANCODE_LEFT)])
	{
		player_->direction = GridDirection::XMinus;
		player_->position[0] -= g_player_speed;
		player_->position[1] = IntToFixed16(Fixed16RoundToInt(player_->position[1]));
	}
	else if(keyboard_state.size() > size_t(SDL_SCANCODE_RIGHT) && keyboard_state[size_t(SDL_SCANCODE_RIGHT)])
	{
		player_->direction = GridDirection::XPlus;
		player_->position[0] += g_player_speed;
		player_->position[1] = IntToFixed16(Fixed16RoundToInt(player_->position[1]));
	}
	else if(keyboard_state.size() > size_t(SDL_SCANCODE_UP) && keyboard_state[size_t(SDL_SCANCODE_UP)])
	{
		player_->direction = GridDirection::YMinus;
		player_->position[1] -= g_player_speed;
		player_->position[0] = IntToFixed16(Fixed16RoundToInt(player_->position[0]));
	}
	else if(keyboard_state.size() > size_t(SDL_SCANCODE_DOWN) && keyboard_state[size_t(SDL_SCANCODE_DOWN)])
	{
		player_->direction = GridDirection::YPlus;
		player_->position[1] += g_player_speed;
		player_->position[0] = IntToFixed16(Fixed16RoundToInt(player_->position[0]));
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
