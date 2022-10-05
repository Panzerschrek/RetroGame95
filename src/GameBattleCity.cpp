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
const fixed16_t g_enemy_speed = g_fixed16_one * 4 / GameInterface::c_update_frequency;
const fixed16_t g_projectile_speed = g_fixed16_one * 20 / GameInterface::c_update_frequency;

const fixed16_t g_tank_half_size = g_fixed16_one;
const fixed16_t g_projectile_half_size = g_fixed16_one / 8;

const size_t g_max_alive_enemies = 3;

uint32_t BlockMaskForCoord(const uint32_t x, const uint32_t y)
{
	assert(x <= 1);
	assert(y <= 1);
	return 1 << (x | (y << 1));
}

using DrawFunc = void(*)(FrameBuffer, SpriteBMP, uint8_t, uint32_t, uint32_t);
DrawFunc GetDrawFuncForDirection(const GridDirection direction)
{
	switch(direction)
	{
	case GridDirection::XMinus:
		return DrawSpriteWithAlphaRotate270;
	case GridDirection::XPlus:
		return DrawSpriteWithAlphaRotate90;
	case GridDirection::YMinus:
		return DrawSpriteWithAlpha;
	case GridDirection::YPlus:
		return DrawSpriteWithAlphaRotate180;
	}
	assert(false);
	return DrawSpriteWithAlpha;
}

} // namespace

GameBattleCity::GameBattleCity(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
	FillField(battle_city_level_0);

	Player player;
	player.position = {IntToFixed16(int32_t(c_field_width / 2 - 4)), IntToFixed16(int32_t(c_field_height - 1))};
	player.direction = GridDirection::YMinus;

	player_ = player;
}

void GameBattleCity::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	++tick_;

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

	for(Enemy& enemy : enemies_)
	{
		UpdateEnemy(enemy);
	}

	for(size_t p = 0; p < projectiles_.size();)
	{
		if(UpdateProjectile(projectiles_[p]))
		{
			// This projectile is dead.
			if(p + 1 < projectiles_.size())
			{
				projectiles_[p] = projectiles_.back();
			}
			projectiles_.pop_back();
		}
		else
		{
			++p;
		}
	} // for projectiles.

	if(enemies_.size() < g_max_alive_enemies && (tick_ % GameInterface::c_update_frequency) == 0)
	{
		SpawnNewEnemy();
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

		const uint32_t sprite_x = field_offset_x + x * c_block_size;
		const uint32_t sprite_y = field_offset_y + y * c_block_size;
		const SpriteBMP sprite = block_sprites[size_t(block.type)];

		if(block.destruction_mask == 0xF)
		{
			DrawSprite(frame_buffer, sprite, sprite_x, sprite_y);
		}
		else
		{
			const uint32_t segment_size = c_block_size / 2;
			for(uint32_t dy = 0; dy < 2; ++dy)
			for(uint32_t dx = 0; dx < 2; ++dx)
			{
				if((block.destruction_mask & BlockMaskForCoord(dx, dy)) != 0)
				{
					DrawSpriteRect(
						frame_buffer,
						sprite,
						sprite_x + dx * segment_size,
						sprite_y + dy * segment_size,
						dx * segment_size,
						dy * segment_size,
						segment_size,
						segment_size);
				}
			}
		}
	}

	if(player_ != std::nullopt)
	{
		const uint32_t x = uint32_t(Fixed16FloorToInt(player_->position[0] * int32_t(c_block_size)));
		const uint32_t y = uint32_t(Fixed16FloorToInt(player_->position[1] * int32_t(c_block_size)));

		const bool use_a = ((x ^ y) & 1) != 0;

		const SpriteBMP sprite(use_a ? Sprites::battle_city_player_0_a : Sprites::battle_city_player_0_b);
		GetDrawFuncForDirection(player_->direction)(
			frame_buffer,
			sprite,
			0,
			field_offset_x + x - sprite.GetWidth () / 2,
			field_offset_y + y - sprite.GetHeight() / 2);
	}

	for(const Enemy& enemy : enemies_)
	{
		const SpriteBMP sprite(Sprites::battle_city_enemy_tank);
		GetDrawFuncForDirection(enemy.direction)(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(enemy.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(enemy.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}

	for(const Projectile& projectile : projectiles_)
	{
		const SpriteBMP sprite(Sprites::battle_city_projectile);
		GetDrawFuncForDirection(projectile.direction)(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(projectile.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(projectile.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
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
	const bool left_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_LEFT) && keyboard_state[size_t(SDL_SCANCODE_LEFT)];
	const bool right_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_RIGHT) && keyboard_state[size_t(SDL_SCANCODE_RIGHT)];
	const bool up_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_UP) && keyboard_state[size_t(SDL_SCANCODE_UP)];
	const bool down_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_DOWN) && keyboard_state[size_t(SDL_SCANCODE_DOWN)];

	// Rotate player.
	// Align player position to grid to simplify navigation.
	if(left_pressed)
	{
		player_->direction = GridDirection::XMinus;
		player_->position[1] = IntToFixed16(Fixed16RoundToInt(player_->position[1]));
	}
	else if(right_pressed)
	{
		player_->direction = GridDirection::XPlus;
		player_->position[1] = IntToFixed16(Fixed16RoundToInt(player_->position[1]));
	}
	else if(up_pressed)
	{
		player_->direction = GridDirection::YMinus;
		player_->position[0] = IntToFixed16(Fixed16RoundToInt(player_->position[0]));
	}
	else if(down_pressed)
	{
		player_->direction = GridDirection::YPlus;
		player_->position[0] = IntToFixed16(Fixed16RoundToInt(player_->position[0]));
	}

	// Move player.
	fixed16vec2_t new_position = player_->position;
	if(left_pressed)
	{
		new_position[0] -= g_player_speed;
	}
	else if(right_pressed)
	{
		new_position[0] += g_player_speed;
	}
	else if(up_pressed)
	{
		new_position[1] -= g_player_speed;
	}
	else if(down_pressed)
	{
		new_position[1] += g_player_speed;
	}

	if(CanMove(
		{new_position[0] - g_tank_half_size, new_position[1] - g_tank_half_size},
		{new_position[0] + g_tank_half_size, new_position[1] + g_tank_half_size}))
	{
		player_->position = new_position;
	}

	// Correct player position - do not allow to move outside field borders.
	const fixed16_t border_x_start = 0 + g_tank_half_size;
	const fixed16_t border_x_end   = IntToFixed16(int32_t(c_field_width )) - g_tank_half_size;
	const fixed16_t border_y_start = 0 + g_tank_half_size;
	const fixed16_t border_y_end   = IntToFixed16(int32_t(c_field_height)) - g_tank_half_size;

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

	// Shoot.
	if(keyboard_state.size() > size_t(SDL_SCANCODE_LCTRL) && keyboard_state[size_t(SDL_SCANCODE_LCTRL)])
	{
		// TODO - fix this - do not allow more than one active projectile.
		if(tick_ >= player_->next_shot_tick)
		{
			player_->next_shot_tick = tick_ + 100;

			Projectile projectile;
			projectile.position = player_->position;
			projectile.direction = player_->direction;

			const fixed16_t offset = g_tank_half_size + g_projectile_half_size;

			switch (projectile.direction)
			{
			case GridDirection::XPlus:
				projectile.position[0] += offset;
				break;
			case GridDirection::XMinus:
				projectile.position[0] -= offset;
				break;
			case GridDirection::YPlus:
				projectile.position[1] += offset;
				break;
			case GridDirection::YMinus:
				projectile.position[1] -= offset;
				break;
			}

			projectiles_.push_back(projectile);
		}
	}
}

bool GameBattleCity::UpdateProjectile(Projectile& projectile)
{
	switch (projectile.direction)
	{
	case GridDirection::XPlus:
		projectile.position[0] += g_projectile_speed;
		break;
	case GridDirection::XMinus:
		projectile.position[0] -= g_projectile_speed;
		break;
	case GridDirection::YPlus:
		projectile.position[1] += g_projectile_speed;
		break;
	case GridDirection::YMinus:
		projectile.position[1] -= g_projectile_speed;
		break;
	}

	const int32_t min_x = Fixed16FloorToInt(projectile.position[0] - g_projectile_half_size);
	const int32_t min_y = Fixed16FloorToInt(projectile.position[1] - g_projectile_half_size);
	const int32_t max_x = Fixed16CeilToInt(projectile.position[0] + g_projectile_half_size);
	const int32_t max_y = Fixed16CeilToInt(projectile.position[1] + g_projectile_half_size);

	if( min_x < 0 || max_x > int32_t(c_field_width ) ||
		min_y < 0 || max_y > int32_t(c_field_height))
	{
		return true;
	}

	bool hit = false;
	for(int32_t y = std::max(0, min_y); y < std::min(max_y, int32_t(c_field_height)); ++y)
	for(int32_t x = std::max(0, min_x); x < std::min(max_x, int32_t(c_field_width )); ++x)
	{
		Block& block = field_[uint32_t(x) + uint32_t(y) * c_field_width];
		if(block.type == BlockType::Empty || block.type == BlockType::Foliage || block.type == BlockType::Water)
		{
			continue;
		}
		if(block.destruction_mask == 0)
		{
			continue;
		}

		if(block.type == BlockType::Bricks)
		{
			uint32_t mask = 0;
			switch(projectile.direction)
			{
			case GridDirection::XPlus:
				mask = BlockMaskForCoord(0, 0) | BlockMaskForCoord(0, 1);
				break;
			case GridDirection::XMinus:
				mask = BlockMaskForCoord(1, 0) | BlockMaskForCoord(1, 1);
				break;
			case GridDirection::YPlus:
				mask = BlockMaskForCoord(0, 0) | BlockMaskForCoord(1, 0);
				break;
			case GridDirection::YMinus:
				mask = BlockMaskForCoord(0, 1) | BlockMaskForCoord(1, 1);
				break;
			}

			if((mask & block.destruction_mask) == 0)
			{
				// This side of block is already destroyed. Destroy another side.
				block.destruction_mask = 0;
			}
			else
			{
				// Destroy only this side.
				block.destruction_mask &= ~mask;
			}
		}

		// TODO - destroy also concrete blocks if projectile is from upgraded player tank.
		hit = true;
	}

	// TODO - process collisions against player and enemies.
	// TODO - process collisions against the base.
	// TODO - make visual/sound effects on collision.

	return hit;
}

void GameBattleCity::UpdateEnemy(Enemy& enemy)
{
	fixed16vec2_t new_position = enemy.position;

	const fixed16_t speed = g_enemy_speed; // TODO - increase dpeed for fast tanks.
	switch (enemy.direction)
	{
	case GridDirection::XPlus:
		new_position[0] += speed;
		break;
	case GridDirection::XMinus:
		new_position[0] -= speed;
		break;
	case GridDirection::YPlus:
		new_position[1] += speed;
		break;
	case GridDirection::YMinus:
		new_position[1] -= speed;
		break;
	}

	if(CanMove(
		{new_position[0] - g_tank_half_size, new_position[1] - g_tank_half_size},
		{new_position[0] + g_tank_half_size, new_position[1] + g_tank_half_size}))
	{
		enemy.position = new_position;
	}
	else
	{
		// Change direction to random.
		// TODO - try to move towards target.

		// Do not allow to preserve direction.
		for(uint32_t i = 0; i < 64; ++i)
		{
			const GridDirection new_direction = GridDirection(rand_.Next() % 4);
			if(enemy.direction != new_direction)
			{
				enemy.direction = new_direction;
				break;
			}
		}

		// Align enemy to grid.
		switch(enemy.direction)
		{
		case GridDirection::XPlus:
		case GridDirection::XMinus:
			enemy.position[1] = IntToFixed16(Fixed16RoundToInt(enemy.position[1]));
			break;
		case GridDirection::YPlus:
		case GridDirection::YMinus:
			enemy.position[0] = IntToFixed16(Fixed16RoundToInt(enemy.position[0]));
			break;
		}
	}

	// TODO - shoot.
}

bool GameBattleCity::CanMove(const fixed16vec2_t& min, const fixed16vec2_t& max) const
{
	const int32_t min_x = Fixed16FloorToInt(min[0]);
	const int32_t min_y = Fixed16FloorToInt(min[1]);
	const int32_t max_x = Fixed16CeilToInt(max[0]);
	const int32_t max_y = Fixed16CeilToInt(max[1]);

	if( min_x < 0 || max_x > int32_t(c_field_width ) ||
		min_y < 0 || max_y > int32_t(c_field_height))
	{
		return false;
	}

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

void GameBattleCity::SpawnNewEnemy()
{
	// Try to spawn it at random position, but avoid obstacles.
	for(uint32_t i = 0; i < 64; ++i)
	{
		const uint32_t x = 1 + (rand_.Next() % (c_field_width - 2));
		const uint32_t y = 1;

		const fixed16vec2_t position = {IntToFixed16(int32_t(x)), IntToFixed16(int32_t(y))};

		if(!CanMove(
			{position[0] - g_tank_half_size, position[1] - g_tank_half_size},
			{position[0] + g_tank_half_size, position[1] + g_tank_half_size}))
		{
			continue;
		}

		Enemy enemy;
		enemy.position = position;
		enemy.direction = GridDirection::YPlus;
		enemies_.push_back(enemy);
		return;
	}
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
