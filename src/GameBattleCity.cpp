#include "GameBattleCity.hpp"
#include "BattleCityLevels.hpp"
#include "Draw.hpp"
#include "GameEndScreen.hpp"
#include "GameMainMenu.hpp"
#include "GamesDrawCommon.hpp"
#include "SpriteBMP.hpp"
#include "Sprites.hpp"
#include "String.hpp"
#include "Strings.hpp"
#include <cassert>
#include <cstring>

namespace
{

const fixed16_t g_player_speed = g_fixed16_one * 4 / GameInterface::c_update_frequency;
const fixed16_t g_enemy_speed = g_fixed16_one * 4 / GameInterface::c_update_frequency;
const fixed16_t g_enemy_speed_fast = g_enemy_speed * 3 / 2;
const fixed16_t g_pacman_ghost_speed = g_player_speed * 9 / 10;

const fixed16_t g_projectile_speed = g_fixed16_one * 20 / GameInterface::c_update_frequency;
const fixed16_t g_armor_piercing_projectile_speed = g_projectile_speed * 2 / 3;

const fixed16_t g_tank_half_size = g_fixed16_one - 1;
const fixed16_t g_projectile_half_size = g_fixed16_one / 8;
const fixed16_t g_snake_bonus_half_size = g_fixed16_one / 2;

const uint32_t g_min_player_reload_interval = GameInterface::c_update_frequency / 3;
const uint32_t g_explosion_duration = GameInterface::c_update_frequency / 3;
const uint32_t g_spawn_shield_duration = GameInterface::c_update_frequency * 3;
const uint32_t g_shield_bonus_duration = GameInterface::c_update_frequency * 15;
const uint32_t g_enemies_freezee_bonus_duration = GameInterface::c_update_frequency * 15;
const uint32_t g_base_protection_bonus_duration = GameInterface::c_update_frequency * 15;
const uint32_t g_enemy_spawn_animation_duration = GameInterface::c_update_frequency;
const uint32_t g_level_start_animation_duration = GameInterface::c_update_frequency * 2;

const size_t g_max_alive_enemies = 3;
const size_t g_max_alive_pacman_ghosts = 2;
const uint32_t g_enemies_per_level = 15;
const uint32_t g_max_lives = 9;

const uint32_t g_transition_time_hide_pacman_ui = GameInterface::c_update_frequency * 3;
const uint32_t g_transition_time_show_field_borders = g_transition_time_hide_pacman_ui + GameInterface::c_update_frequency;
const uint32_t g_transition_time_stop_auto_movement = g_transition_time_show_field_borders + GameInterface::c_update_frequency;
const uint32_t g_transition_time_show_tank = g_transition_time_stop_auto_movement + GameInterface::c_update_frequency;
const uint32_t g_transition_time_show_field = g_transition_time_show_tank + GameInterface::c_update_frequency;
const uint32_t g_transition_time_show_ui = g_transition_time_show_field + GameInterface::c_update_frequency;

bool BBoxesIntersect(
	const fixed16vec2_t& min_a, const fixed16vec2_t& max_a,
	const fixed16vec2_t& min_b, const fixed16vec2_t& max_b)
{
	return !(
		min_a[0] >= max_b[0] || max_a[0] <= min_b[0] ||
		min_a[1] >= max_b[1] || max_a[1] <= min_b[1]);
}

bool TanksIntersects(const fixed16vec2_t& pos0, const fixed16vec2_t& pos1)
{
	return BBoxesIntersect(
		{pos0[0] - g_tank_half_size, pos0[1] - g_tank_half_size},
		{pos0[0] + g_tank_half_size, pos0[1] + g_tank_half_size},
		{pos1[0] - g_tank_half_size, pos1[1] - g_tank_half_size},
		{pos1[0] + g_tank_half_size, pos1[1] + g_tank_half_size});
}

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
	OpenGame(GameId::BattleCity);

	NextLevel();
	level_start_animation_end_tick_ = 0;

	SpawnSnakeBonus();
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

	UpdateBaseProtectionBonus();

	if(!game_over_ && tick_ > level_start_animation_end_tick_ && tick_ >= level_end_animation_end_tick_)
	{
		if(player_ == std::nullopt)
		{
			// Respawn player.
			if(lives_ == 0)
			{
				game_over_ = true;
			}
			else
			{
				--lives_;
				SpawnPlayer();
			}
		}

		if(player_ != std::nullopt)
		{
			ProcessPlayerInput(keyboard_state);
			TryToPickUpBonus();
			TryToPickUpSnakeBonus();
		}

		for(Enemy& enemy : enemies_)
		{
			UpdateEnemy(enemy);
			if(enemy.projectile != std::nullopt)
			{
				if(UpdateProjectile(*enemy.projectile, false))
				{
					enemy.projectile = std::nullopt;
				}
			}
		}

		for(PacmanGhost& pacman_ghost : pacman_ghosts_)
		{
			UpdatePacmanGhost(pacman_ghost);
		}

		if(player_ != std::nullopt)
		{
			for(size_t p = 0; p < player_->projectiles.size();)
			{
				if(UpdateProjectile(player_->projectiles[p], true))
				{
					// This projectile is dead.
					if(p + 1 < player_->projectiles.size())
					{
						player_->projectiles[p] = player_->projectiles.back();
					}
					player_->projectiles.pop_back();
				}
				else
				{
					++p;
				}
			} // for projectiles.
		}

		if(tick_ >= g_transition_time_show_tank &&
			enemies_left_ > 0 &&
			enemies_.size() < g_max_alive_enemies &&
			(tick_ % GameInterface::c_update_frequency) == 0)
		{
			SpawnNewEnemy();
		}

		TrySpawnExtraElement();
	}

	for(size_t e = 0; e < explosions_.size();)
	{
		if(tick_ >= explosions_[e].start_tick + g_explosion_duration)
		{
			// This explosion is dead.
			if(e + 1 < explosions_.size())
			{
				explosions_[e] = explosions_.back();
			}
			explosions_.pop_back();
		}
		else
		{
			++e;
		}
	} // for explosions.

	if(tick_ == level_end_animation_end_tick_)
	{
		NextLevel();
	}
	if(enemies_.empty() && enemies_left_ == 0 && !base_is_destroyed_ && player_ != std::nullopt && !game_over_)
	{
		EndLevel();
	}
}

void GameBattleCity::Draw(const FrameBuffer frame_buffer) const
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_width  = c_field_width  * c_block_size;
	const uint32_t field_height = c_field_height * c_block_size;

	const uint32_t field_offset_x = c_block_size * 2;
	const uint32_t field_offset_y = (frame_buffer.height - field_height) / 2;

	static constexpr const SpriteBMP block_sprites[]
	{
		Sprites::battle_city_block_bricks,
		Sprites::battle_city_block_bricks,
		Sprites::battle_city_block_concrete,
		Sprites::battle_city_block_foliage,
		Sprites::battle_city_block_water,
		Sprites::tetris_block_small_4,
		Sprites::tetris_block_small_7,
		Sprites::tetris_block_small_5,
		Sprites::tetris_block_small_1,
		Sprites::tetris_block_small_2,
		Sprites::tetris_block_small_6,
		Sprites::tetris_block_small_3,
	};

	if(tick_ < g_transition_time_show_field)
	{
		// Base.
		for(uint32_t dy = 0; dy < 2; ++dy)
		for(uint32_t dx = 0; dx < 2; ++dx)
		{
			const SpriteBMP sprite(Sprites::pacman_food);
			DrawSpriteWithAlpha(
				frame_buffer,
				sprite,
				0,
				field_offset_x + c_block_size * (c_field_width / 2 - 1 + dx) + c_block_size / 2 - sprite.GetWidth() / 2,
				field_offset_y + c_block_size * (c_field_height - 2 + dy)  + c_block_size / 2 - sprite.GetHeight() / 2);
		}

		const uint32_t pacman_field_width  = c_field_width  + 4;
		const uint32_t pacman_field_height = c_field_height + 4;
		DrawPacmanField(
			frame_buffer,
			battle_city_level_0_pacman_field,
			pacman_field_width,
			pacman_field_height,
			0,
			0,
			pacman_field_width,
			pacman_field_height);
	}
	else
	{
		// Base.
		DrawSpriteWithAlpha(
			frame_buffer,
			base_is_destroyed_ ? Sprites::battle_city_eagle_destroyed : Sprites::battle_city_eagle,
			0,
			field_offset_x + c_block_size * (c_field_width / 2 - 1),
			field_offset_y + c_block_size * (c_field_height - 2));

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
	}

	const Color32 border_color = g_cga_palette[8];
	if(tick_ >= g_transition_time_show_field_borders)
	{
		const uint32_t field_x_end = field_offset_x + field_width ;
		const uint32_t field_y_end = field_offset_y + field_height;

		FillRect(frame_buffer, border_color, 0, 0, frame_buffer.width, field_offset_y);
		FillRect(frame_buffer, border_color, 0, field_offset_y + field_height, frame_buffer.width, frame_buffer.height - field_y_end);
		FillRect(frame_buffer, border_color, 0, field_offset_y, field_offset_x, field_height);
		FillRect(frame_buffer, border_color, field_x_end, field_offset_y, frame_buffer.width - field_x_end, field_height);
	}

	if(snake_bonus_ != std::nullopt)
	{
		static constexpr const SpriteBMP bonus_sprites[]
		{
			Sprites::snake_food_small,
			Sprites::snake_food_medium,
		};

		const SpriteBMP sprite =
			tick_ < g_transition_time_show_field
				? Sprites::pacman_bonus_deadly
				: bonus_sprites[size_t(snake_bonus_->type)];
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(snake_bonus_->position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(snake_bonus_->position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}

	if(player_ != std::nullopt)
	{
		const uint32_t x = uint32_t(Fixed16FloorToInt(player_->position[0] * int32_t(c_block_size)));
		const uint32_t y = uint32_t(Fixed16FloorToInt(player_->position[1] * int32_t(c_block_size)));

		if(tick_ < g_transition_time_show_tank)
		{
			static constexpr const SpriteBMP sprites[]
			{
				Sprites::pacman_0,
				Sprites::pacman_1,
				Sprites::pacman_2,
				Sprites::pacman_3,
				Sprites::pacman_2,
				Sprites::pacman_1,
			};

			const uint32_t num_frames = uint32_t(std::size(sprites));
			const fixed16_t dist = player_->position[0] + player_->position[1];

			const SpriteBMP sprite(sprites[((uint32_t(dist) * num_frames) >> g_fixed16_base) % num_frames]);
			const uint32_t pacman_x = field_offset_x + x - sprite.GetWidth () / 2;
			const uint32_t pacman_y = field_offset_y + y - sprite.GetHeight() / 2;
			switch(player_->direction)
			{
			case GridDirection::XMinus:
				DrawSpriteWithAlphaRotate180(frame_buffer, sprite, 0, pacman_x, pacman_y);
				break;
			case GridDirection::XPlus:
				DrawSpriteWithAlpha         (frame_buffer, sprite, 0, pacman_x, pacman_y);
				break;
			case GridDirection::YMinus:
				DrawSpriteWithAlphaRotate270(frame_buffer, sprite, 0, pacman_x, pacman_y);
				break;
			case GridDirection::YPlus:
				DrawSpriteWithAlphaRotate90 (frame_buffer, sprite, 0, pacman_x, pacman_y);
				break;
			}
		}
		else
		{
			{
				const bool use_a = ((x ^ y) & 1) != 0;

				const SpriteBMP sprite(use_a ? Sprites::battle_city_player_0_a : Sprites::battle_city_player_0_b);
				GetDrawFuncForDirection(player_->direction)(
					frame_buffer,
					sprite,
					0,
					field_offset_x + x - sprite.GetWidth () / 2,
					field_offset_y + y - sprite.GetHeight() / 2);
			}

			if(tick_ < player_->shield_end_tick)
			{
				const SpriteBMP sprite(tick_ / 6 % 2 != 0 ? Sprites::battle_city_player_shield_a : Sprites::battle_city_player_shield_b);
				DrawSpriteWithAlpha(
					frame_buffer,
					sprite,
					0,
					field_offset_x + x - sprite.GetWidth () / 2,
					field_offset_y + y - sprite.GetHeight() / 2);
			}
		}
	}

	static constexpr const SpriteBMP spawn_glow_sprites[]
	{
		Sprites::battle_city_tank_spawn_glow_0,
		Sprites::battle_city_tank_spawn_glow_1,
		Sprites::battle_city_tank_spawn_glow_2,
		Sprites::battle_city_tank_spawn_glow_1,
		Sprites::battle_city_tank_spawn_glow_0,
		Sprites::battle_city_tank_spawn_glow_1,
		Sprites::battle_city_tank_spawn_glow_2,
		Sprites::battle_city_tank_spawn_glow_1,
		Sprites::battle_city_tank_spawn_glow_0,
	};
	const auto num_spawn_glow_sprites = uint32_t(std::size(spawn_glow_sprites));

	for(const Enemy& enemy : enemies_)
	{
		const uint32_t x = uint32_t(Fixed16FloorToInt(enemy.position[0] * int32_t(c_block_size)));
		const uint32_t y = uint32_t(Fixed16FloorToInt(enemy.position[1] * int32_t(c_block_size)));

		if(tick_ < enemy.spawn_tick + g_enemy_spawn_animation_duration)
		{
			uint32_t i = std::min((tick_ - enemy.spawn_tick) * num_spawn_glow_sprites / g_enemy_spawn_animation_duration, num_spawn_glow_sprites - 1);
			const SpriteBMP sprite = spawn_glow_sprites[i];

			GetDrawFuncForDirection(enemy.direction)(
				frame_buffer,
				sprite,
				0,
				field_offset_x + x - sprite.GetWidth () / 2,
				field_offset_y + y - sprite.GetHeight() / 2);
		}
		else
		{
			const bool use_a = ((x ^ y) & 1) != 0;

			SpriteBMP sprite(Sprites::battle_city_enemy_0_a);
			switch(enemy.type)
			{
			case EnemyType::Basic:
				sprite = use_a ? Sprites::battle_city_enemy_0_a : Sprites::battle_city_enemy_0_b;
				break;
			case EnemyType::LessRandom:
				sprite = use_a ? Sprites::battle_city_enemy_1_a : Sprites::battle_city_enemy_1_b;
				break;
			case EnemyType::Fast:
				sprite = use_a ? Sprites::battle_city_enemy_2_a : Sprites::battle_city_enemy_2_b;
				break;
			case EnemyType::Heavy:
				sprite = use_a ? Sprites::battle_city_enemy_3_a : Sprites::battle_city_enemy_3_b;
				break;
			case EnemyType::NumTypes:
				assert(false);
				break;
			};

			const uint32_t start_x = field_offset_x + x - sprite.GetWidth () / 2;
			const uint32_t start_y = field_offset_y + y - sprite.GetHeight() / 2;
			if(enemy.gives_bonus)
			{
				const uint8_t colors[]= {0, 4, 12, 15};
				uint32_t i = tick_ / 6 % 4;
				if(i != 0)
				{
					FillRect(
						frame_buffer,
						g_cga_palette[colors[i]],
						start_x,
						start_y,
						sprite.GetWidth(),
						sprite.GetHeight());
				}
			}

			GetDrawFuncForDirection(enemy.direction)( frame_buffer, sprite, 0, start_x, start_y);
		}
	}

	for(const PacmanGhost& pacman_ghost : pacman_ghosts_)
	{
		const SpriteBMP sprite = GetPacmanGhostSprite(pacman_ghost.type, pacman_ghost.direction);
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(pacman_ghost.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(pacman_ghost.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}

	const auto draw_projectile =
	[&](const Projectile& projectile)
	{
		const SpriteBMP sprite(projectile.is_armor_piercing ? Sprites::arkanoid_ball : Sprites::battle_city_projectile);
		GetDrawFuncForDirection(projectile.direction)(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(projectile.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(projectile.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	};

	if(player_ != std::nullopt)
	{
		for(const Projectile& projectile : player_->projectiles)
		{
			draw_projectile(projectile);
		}
	}
	for(const Enemy& enemy : enemies_)
	{
		if(enemy.projectile != std::nullopt)
		{
			draw_projectile(*enemy.projectile);
		}
	}

	static constexpr const SpriteBMP explosion_sprites[]
	{
		Sprites::battle_city_explosion_0,
		Sprites::battle_city_explosion_1,
		Sprites::battle_city_explosion_2,
		Sprites::battle_city_explosion_2,
		Sprites::battle_city_explosion_1,
		Sprites::battle_city_explosion_0
	};
	const auto num_explosion_sprites = uint32_t(std::size(explosion_sprites));

	for(const Explosion& explosion : explosions_)
	{
		uint32_t i = std::min((tick_ - explosion.start_tick) * num_explosion_sprites / g_explosion_duration, num_explosion_sprites - 1);
		const SpriteBMP sprite = explosion_sprites[i];

		// Use pseudo-random rotation.
		GetDrawFuncForDirection(GridDirection(explosion.start_tick % 4))(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(explosion.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(explosion.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}

	if(tick_ >= g_transition_time_show_field)
	{
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
	}

	if(bonus_ != std::nullopt)
	{
		static constexpr const SpriteBMP bonus_sprites[]
		{
			Sprites::battle_city_bonus_tank,
			Sprites::battle_city_bonus_star,
			Sprites::battle_city_bonus_helmet,
			Sprites::battle_city_bonus_shovel,
			Sprites::battle_city_bonus_grenade,
			Sprites::battle_city_bonus_clock,
		};

		const SpriteBMP sprite = bonus_sprites[size_t(bonus_->type)];
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(bonus_->position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(bonus_->position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}

	// UI.

	if(tick_ < g_transition_time_hide_pacman_ui)
	{
		const SpriteBMP life_spirte(Sprites::pacman_life);
		for(uint32_t i = 0; i < lives_; ++i)
		{
			DrawSpriteWithAlpha(
				frame_buffer,
				life_spirte,
				0,
				field_offset_x + (c_field_width + 1) * c_block_size + i % 3 * (life_spirte.GetWidth() + 2),
				c_block_size * 2 + i / 3 * (life_spirte.GetHeight() + 3));
		}

		const uint32_t texts_offset_x = field_offset_x + (c_field_width + 1) * c_block_size;
		const uint32_t texts_offset_y = 8 * g_glyph_height;

		DrawText(frame_buffer, g_cga_palette[10], texts_offset_x, texts_offset_y + 0 * g_glyph_height, Strings::pacman_level);

		char text[64];
		NumToString(text, sizeof(text), level_, 5);
		DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y + 2 * g_glyph_height, text);

		DrawText(frame_buffer, g_cga_palette[10], texts_offset_x, texts_offset_y + 5 * g_glyph_height, Strings::pacman_score);
		DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y + 7 * g_glyph_height, "    ß");
	}

	if(tick_ >= g_transition_time_show_ui)
	{
		for(uint32_t i = 0; i < enemies_left_; ++i)
		{
			const SpriteBMP sprite(Sprites::battle_city_remaining_enemy);
			DrawSprite(
				frame_buffer,
				sprite,
				frame_buffer.width - 32 + sprite.GetWidth() * (i % 2),
				40 + (i / 2) * sprite.GetHeight());
		}

		if(player_ != std::nullopt)
		{
			for(uint32_t i = 0; i < player_->armor_piercing_shells; ++i)
			{
				const SpriteBMP sprite(Sprites::arkanoid_ball);
				DrawSprite(
					frame_buffer,
					sprite,
					frame_buffer.width - 40 + (sprite.GetWidth() + 2) * (i % 4),
					frame_buffer.height - 24 + (i / 4) * (sprite.GetHeight() + 2));
			}
		}

		const uint32_t texts_offset_x = frame_buffer.width - 32;
		const uint32_t texts_offset_y = frame_buffer.height / 2 + g_glyph_height;
		char text[16];

		DrawText(frame_buffer, g_color_black, texts_offset_x, texts_offset_y, "IP");
		NumToString(text, sizeof(text), lives_, 2);
		DrawSprite(frame_buffer, Sprites::battle_city_player_lives_symbol, texts_offset_x, texts_offset_y + g_glyph_height);
		DrawText(frame_buffer, g_color_black, texts_offset_x, texts_offset_y + g_glyph_height, text);

		NumToString(text, sizeof(text), level_, 2);
		DrawText(frame_buffer, g_color_black, texts_offset_x, texts_offset_y + g_glyph_height * 8, text);
		DrawSprite(frame_buffer, Sprites::battle_city_level_symbol, texts_offset_x, texts_offset_y + g_glyph_height * 6);
	}

	if(game_over_)
	{
		DrawTextCenteredWithOutline(
			frame_buffer,
			g_cga_palette[4],
			g_cga_palette[15],
			field_offset_x + c_field_width  * c_block_size / 2,
			field_offset_y + c_field_height * c_block_size / 2,
			Strings::battle_city_game_over);
	}
	else if(tick_ < level_end_animation_end_tick_)
	{
		const uint32_t fade_duration = GameInterface::c_update_frequency * 5;
		const uint32_t y_offset =
			field_height * (std::min(fade_duration, level_end_animation_end_tick_ - tick_)) / fade_duration;

		FillRect(frame_buffer, border_color, field_offset_x, field_offset_y + y_offset, field_width, field_height - y_offset);
	}
	else if(tick_ < level_start_animation_end_tick_)
	{
		const uint32_t y_offset =
			field_height - field_height * (std::min(g_level_start_animation_duration, level_start_animation_end_tick_ - tick_)) / g_level_start_animation_duration;

		FillRect(frame_buffer, border_color, field_offset_x, field_offset_y + y_offset, field_width, field_height - y_offset);

		const uint32_t text_y_center = field_offset_y + c_field_height * c_block_size / 2;
		if(field_offset_y + y_offset < text_y_center)
		{
			char text[64];
			std::strcpy(text, Strings::battle_city_stage);
			const size_t offset = std::strlen(text);
			NumToString(text + offset, std::size(text) - offset, level_, 2);

			DrawTextCentered(
				frame_buffer,
				g_cga_palette[0],
				field_offset_x + c_field_width  * c_block_size / 2,
				field_offset_y + c_field_height * c_block_size / 2,
				text);
		}
	}
}

GameInterfacePtr GameBattleCity::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameBattleCity::EndLevel()
{
	if(tick_ < level_end_animation_end_tick_)
	{
		return;
	}

	const MusicId music_id = MusicId::PreussensGloria;
	sound_player_.PlayMusic(music_id);

	level_end_animation_end_tick_ =
		tick_ +
		uint32_t(Fixed16RoundToInt(g_fixed16_one + sound_player_.GetMelodyDuration(music_id))) * GameInterface::c_update_frequency;
}

void GameBattleCity::NextLevel()
{
	const auto max_level = uint32_t(std::size(battle_city_levels));
	if(level_ >= max_level)
	{
		if(next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameEndScreen>(sound_player_);
		}
		return;
	}

	++level_;

	lives_ = std::max(lives_, 3u);
	enemies_.clear();
	enemies_left_ = g_enemies_per_level;
	explosions_.clear();
	pacman_ghosts_.clear();
	bonus_ = std::nullopt;
	snake_bonus_ = std::nullopt;
	extra_elements_spawn_points_ = 0;
	enemies_freezee_bonus_end_tick_ = 0;
	base_protection_bonus_end_tick_ = 0;

	level_start_animation_end_tick_ = tick_ + g_level_start_animation_duration;

	FillField(battle_city_levels[level_ - 1]);

	SpawnPlayer();
}

void GameBattleCity::ProcessPlayerInput(const std::vector<bool>& keyboard_state)
{
	const bool left_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_LEFT) && keyboard_state[size_t(SDL_SCANCODE_LEFT)];
	const bool right_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_RIGHT) && keyboard_state[size_t(SDL_SCANCODE_RIGHT)];
	const bool up_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_UP) && keyboard_state[size_t(SDL_SCANCODE_UP)];
	const bool down_pressed = keyboard_state.size() > size_t(SDL_SCANCODE_DOWN) && keyboard_state[size_t(SDL_SCANCODE_DOWN)];

	if(tick_ >= g_transition_time_show_tank)
	{
		if((left_pressed || right_pressed || up_pressed || down_pressed))
		{
			if(current_sound_ == std::nullopt || tick_ >= current_sound_->end_tick || current_sound_->id == SoundId::TankStay)
			{
				sound_player_.PlayLoopedSound(SoundId::TankMovement);
				ActiveSound active_sound;
				active_sound.id = SoundId::TankMovement;
				active_sound.end_tick = std::numeric_limits<uint32_t>::max();
				current_sound_ = active_sound;
			}
		}
		else
		{
			if(current_sound_ == std::nullopt || tick_ >= current_sound_->end_tick || current_sound_->id == SoundId::TankMovement)
			{
				sound_player_.PlayLoopedSound(SoundId::TankStay);
				ActiveSound active_sound;
				active_sound.id = SoundId::TankStay;
				active_sound.end_tick = std::numeric_limits<uint32_t>::max();
				current_sound_ = active_sound;
			}
		}
	}

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
	else if(tick_ < g_transition_time_stop_auto_movement)
	{
		switch(player_->direction)
		{
		case GridDirection::XPlus:
			new_position[0] += g_player_speed;
			break;
		case GridDirection::XMinus:
			new_position[0] -= g_player_speed;
			break;
		case GridDirection::YPlus:
			new_position[1] += g_player_speed;
			break;
		case GridDirection::YMinus:
			new_position[1] -= g_player_speed;
			break;
		}
	}

	bool moves_towards_other_tank = false;
	for(const Enemy& enemy : enemies_)
	{
		if(TanksIntersects(new_position, enemy.position))
		{
			moves_towards_other_tank = true;
			break;
		}
	}

	if(!moves_towards_other_tank && CanMove(new_position))
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
	if(((keyboard_state.size() > size_t(SDL_SCANCODE_LCTRL) && keyboard_state[size_t(SDL_SCANCODE_LCTRL)]) ||
		(keyboard_state.size() > size_t(SDL_SCANCODE_RCTRL) && keyboard_state[size_t(SDL_SCANCODE_RCTRL)]) ||
		(keyboard_state.size() > size_t(SDL_SCANCODE_SPACE) && keyboard_state[size_t(SDL_SCANCODE_SPACE)])) &&
		tick_ >= g_transition_time_show_tank)
	{
		const size_t max_active_projectiles = player_level_;
		if(tick_ >= player_->next_shot_tick && player_->projectiles.size() < max_active_projectiles)
		{
			bool is_armor_piercing = false;
			if(player_->armor_piercing_shells > 0)
			{
				--player_->armor_piercing_shells;
				is_armor_piercing = true;
			}

			player_->next_shot_tick = tick_ + g_min_player_reload_interval;
			player_->projectiles.push_back(MakeProjectile(player_->position, player_->direction, is_armor_piercing));

			MakeEventSound(SoundId::TankShot);
		}
	}
}

void GameBattleCity::TryToPickUpBonus()
{
	assert(player_ != std::nullopt);

	if(bonus_ == std::nullopt)
	{
		return;
	}

	if(!TanksIntersects(player_->position, bonus_->position))
	{
		// Too far.
		return;
	}

	MakeEventSound(SoundId::SnakeBonusEat);

	bool bonus_enemy_destroyed = false;

	// Pick up the bonus.
	switch(bonus_->type)
	{
	case BonusType::ExtraLife:
		lives_ = std::min(lives_ + 1, g_max_lives);
		break;

	case BonusType::TankUpgrade:
		++player_level_;
		break;

	case BonusType::Shield:
		player_->shield_end_tick = tick_ + g_shield_bonus_duration;
		break;

	case BonusType::BaseProtection:
		ActivateBaseProtectionBonus();
		break;

	case BonusType::DestryAllTanks:
		for(const Enemy& enemy : enemies_)
		{
			bonus_enemy_destroyed |= enemy.gives_bonus;
			MakeExplosion(enemy.position);
		}
		extra_elements_spawn_points_ += uint32_t(enemies_.size());
		enemies_.clear();
		break;

	case BonusType::PauseAllTanks:
		enemies_freezee_bonus_end_tick_ = tick_ + g_enemies_freezee_bonus_duration;
		break;

	case BonusType::NumTypes:
		assert(false);
		break;
	}

	bonus_ = std::nullopt;

	if(bonus_enemy_destroyed)
	{
		SpawnBonus();
	}
}

void GameBattleCity::TryToPickUpSnakeBonus()
{
	assert(player_ != std::nullopt);

	if(snake_bonus_ == std::nullopt)
	{
		return;
	}

	if(!BBoxesIntersect(
		{snake_bonus_->position[0] - g_snake_bonus_half_size, snake_bonus_->position[1] - g_snake_bonus_half_size},
		{snake_bonus_->position[0] + g_snake_bonus_half_size, snake_bonus_->position[1] + g_snake_bonus_half_size},
		{player_->position[0] - g_tank_half_size, player_->position[1] - g_tank_half_size},
		{player_->position[0] + g_tank_half_size, player_->position[1] + g_tank_half_size}))
	{
		// Too far.
		return;
	}

	MakeEventSound(SoundId::SnakeBonusEat);

	// Pick up the bonus.
	switch(snake_bonus_->type)
	{
	case SnakeBonusType::FoodSmall:
		BlockEnemiesWithTetrisFigures();
		break;

	case SnakeBonusType::FoodMedium:
		player_->armor_piercing_shells = 4;
		break;

	case SnakeBonusType::NumTypes:
		assert(false);
		break;
	};

	snake_bonus_ = std::nullopt;
}

void GameBattleCity::UpdateEnemy(Enemy& enemy)
{
	const fixed16vec2_t base_pos = {IntToFixed16(int32_t(c_field_width / 2)), IntToFixed16(c_field_height - 1)};

	if(tick_ < enemy.spawn_tick + g_enemy_spawn_animation_duration)
	{
		return;
	}
	if(tick_ < enemies_freezee_bonus_end_tick_)
	{
		return;
	}

	fixed16vec2_t new_position = enemy.position;

	const fixed16_t speed = enemy.type == EnemyType::Fast ? g_enemy_speed_fast : g_enemy_speed;
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

	bool movement_blocked = false;
	for(const Enemy& other_enemy : enemies_)
	{
		if(&enemy != &other_enemy && TanksIntersects(new_position, other_enemy.position))
		{
			movement_blocked = true;
			break;
		}
	}
	for(const PacmanGhost& pacman_ghost : pacman_ghosts_)
	{
		movement_blocked |= TanksIntersects(new_position, pacman_ghost.position);
	}
	if(player_ != std::nullopt)
	{
		movement_blocked |= TanksIntersects(new_position, player_->position);
	}

	// Move straight, but after tile change try to change direction with small probability even if still can move straight.
	const bool tile_changed =
		Fixed16FloorToInt(new_position[0]) != Fixed16FloorToInt(enemy.position[0]) ||
		Fixed16FloorToInt(new_position[1]) != Fixed16FloorToInt(enemy.position[1]);
	const bool want_to_change_direction = tile_changed && rand_.Next() % 16 == 0;

	if(CanMove(new_position) && !movement_blocked && !want_to_change_direction)
	{
		enemy.position = new_position;
	}
	else
	{
		GridDirection new_direction = enemy.direction;

		// LessRandom enemy chooses to move towards target with 80% chance. Other enemies - with only 50%.
		const uint32_t target_move_chance = enemy.type == EnemyType::LessRandom ? 80 : 50;

		if(rand_.Next() % 100 < target_move_chance)
		{
			// Try to move towards target.
			// Choose closest target - base or player.

			fixed16vec2_t target_pos = base_pos;
			const fixed16_t target_pseudo_dist =
				Fixed16Abs(target_pos[0] - enemy.position[0]) + Fixed16Abs(target_pos[1] - enemy.position[1]);

			// Fast and Basic enemies do not use player as target.
			if(player_ != std::nullopt && enemy.type != EnemyType::Fast && enemy.type != EnemyType::Basic)
			{
				const fixed16_t player_pseudo_dist =
					Fixed16Abs(player_->position[0] - enemy.position[0]) + Fixed16Abs(player_->position[1] - enemy.position[1]);
				if(player_pseudo_dist < target_pseudo_dist)
				{
					target_pos = player_->position;
				}
			}

			const fixed16vec2_t vec_to_target = {target_pos[0] - enemy.position[0], target_pos[1] - enemy.position[1]};
			if(Fixed16Abs(vec_to_target[0]) >= Fixed16Abs(vec_to_target[1]))
			{
				new_direction = vec_to_target[0] > 0 ? GridDirection::XPlus : GridDirection::XMinus;
			}
			else
			{
				new_direction = vec_to_target[1] > 0 ? GridDirection::YPlus : GridDirection::YMinus;
			}
		}
		else
		{
			// Change direction to random.
			// Do not allow to preserve direction.
			for(uint32_t i = 0; i < 64; ++i)
			{
				new_direction = GridDirection(rand_.Next() % 4);
				if(enemy.direction != new_direction)
				{
					break;
				}
			}

		}

		// Align enemy to grid.
		new_position = enemy.position;
		switch(new_direction)
		{
		case GridDirection::XPlus:
		case GridDirection::XMinus:
			new_position[1] = IntToFixed16(Fixed16RoundToInt(new_position[1]));
			break;
		case GridDirection::YPlus:
		case GridDirection::YMinus:
			new_position[0] = IntToFixed16(Fixed16RoundToInt(new_position[0]));
			break;
		}

		// Make sure we still do not move towards other tank even after alignemnt to grid.
		movement_blocked = false;
		for(const Enemy& other_enemy : enemies_)
		{
			if(&enemy != &other_enemy && TanksIntersects(new_position, other_enemy.position))
			{
				movement_blocked = true;
				break;
			}
		}
		for(const PacmanGhost& pacman_ghost : pacman_ghosts_)
		{
			movement_blocked |= TanksIntersects(new_position, pacman_ghost.position);
		}
		if(player_ != std::nullopt)
		{
			movement_blocked |= TanksIntersects(new_position, player_->position);
		}

		if(!movement_blocked)
		{
			enemy.position = new_position;
			enemy.direction = new_direction;
		}
	}

	if(enemy.projectile == std::nullopt && rand_.Next() % 147 == 5)
	{
		bool can_fire = true;
		if(enemy.type == EnemyType::Heavy)
		{
			// Heavy tank fires only if player or base is at front of it.

			const auto target_is_in_front =
			[&](const fixed16vec2_t& target) -> bool
			{
				const fixed16_t strafe_threshold = g_fixed16_one * 2;
				switch(enemy.direction)
				{
				case GridDirection::XPlus:
					return target[0] > enemy.position[0] && Fixed16Abs(enemy.position[1] - target[1]) < strafe_threshold;
				case GridDirection::XMinus:
					return target[0] < enemy.position[0] && Fixed16Abs(enemy.position[1] - target[1]) < strafe_threshold;
				case GridDirection::YPlus:
					return target[1] > enemy.position[1] && Fixed16Abs(enemy.position[0] - target[0]) < strafe_threshold;
				case GridDirection::YMinus:
					return target[1] < enemy.position[1] && Fixed16Abs(enemy.position[0] - target[0]) < strafe_threshold;
				};
				assert(false);
				return false;
			};

			can_fire = target_is_in_front(base_pos);
			if(player_ != std::nullopt)
			{
				can_fire |= target_is_in_front(player_->position);
			}
		}

		if(can_fire)
		{
			enemy.projectile = MakeProjectile(enemy.position, enemy.direction, false);
		}
	}
}

void GameBattleCity::UpdatePacmanGhost(PacmanGhost& pacman_ghost)
{
	if(tick_ < enemies_freezee_bonus_end_tick_)
	{
		return;
	}

	const auto movement_is_blocked =
	[&](const fixed16vec2_t& new_position)
	{
		for(const Enemy& enemy : enemies_)
		{
			if(TanksIntersects(new_position, enemy.position))
			{
				return true;
			}
		}
		for(const PacmanGhost& other_pacman_ghost : pacman_ghosts_)
		{
			if(&pacman_ghost != &other_pacman_ghost && TanksIntersects(new_position, other_pacman_ghost.position))
			{
				return true;
			}
		}
		return false;
	};

	fixed16vec2_t new_position = pacman_ghost.position;
	switch(pacman_ghost.direction)
	{
	case GridDirection::XPlus:
		new_position[0] += g_pacman_ghost_speed;
		break;
	case GridDirection::XMinus:
		new_position[0] -= g_pacman_ghost_speed;
		break;
	case GridDirection::YPlus:
		new_position[1] += g_pacman_ghost_speed;
		break;
	case GridDirection::YMinus:
		new_position[1] -= g_pacman_ghost_speed;
		break;
	}

	// Move straight, but after tile change try to change direction.
	const bool tile_changed =
		Fixed16FloorToInt(new_position[0]) != Fixed16FloorToInt(pacman_ghost.position[0]) ||
		Fixed16FloorToInt(new_position[1]) != Fixed16FloorToInt(pacman_ghost.position[1]);

	const bool can_move = CanMove(new_position) && !movement_is_blocked(new_position);
	if(can_move)
	{
		pacman_ghost.position = new_position;
	}

	if((tile_changed || !can_move) && player_ != std::nullopt)
	{
		GridDirection new_direction = pacman_ghost.direction;
		fixed16_t best_square_distance = std::numeric_limits<fixed16_t>::max();

		const auto add_new_direction_candidate =
		[&](const fixed16vec2_t& next_position, const GridDirection direction)
		{
			if(!CanMove(next_position) || movement_is_blocked(next_position))
			{
				return;
			}

			const fixed16_t square_distance =
				Fixed16VecSquareLen(
					{next_position[0] - player_->position[0], next_position[1] - player_->position[1]});
			if(square_distance < best_square_distance)
			{
				best_square_distance = square_distance;
				new_direction = direction;
			}
		};

		// Avoid turning back.
		if(pacman_ghost.direction != GridDirection::XPlus )
		{
			add_new_direction_candidate({pacman_ghost.position[0] - g_fixed16_one / 2, IntToFixed16(Fixed16RoundToInt(pacman_ghost.position[1]))}, GridDirection::XMinus);
		}
		if(pacman_ghost.direction != GridDirection::XMinus)
		{
			add_new_direction_candidate({pacman_ghost.position[0] + g_fixed16_one / 2, IntToFixed16(Fixed16RoundToInt(pacman_ghost.position[1]))}, GridDirection::XPlus );
		}
		if(pacman_ghost.direction != GridDirection::YPlus )
		{
			add_new_direction_candidate({IntToFixed16(Fixed16RoundToInt(pacman_ghost.position[0])), pacman_ghost.position[1] - g_fixed16_one / 2}, GridDirection::YMinus);
		}
		if(pacman_ghost.direction != GridDirection::YMinus)
		{
			add_new_direction_candidate({IntToFixed16(Fixed16RoundToInt(pacman_ghost.position[0])), pacman_ghost.position[1] + g_fixed16_one / 2}, GridDirection::YPlus );
		}

		if(best_square_distance == std::numeric_limits<fixed16_t>::max())
		{
			// Can't move - turn back.
			switch(pacman_ghost.direction)
			{
			case GridDirection::XPlus:
				new_direction = GridDirection::XMinus;
				break;
			case GridDirection::XMinus:
				new_direction = GridDirection::XPlus;
				break;
			case GridDirection::YPlus:
				new_direction = GridDirection::YMinus;
				break;
			case GridDirection::YMinus:
				new_direction = GridDirection::YPlus;
				break;
			}
		}

		// Align ghost to grid.
		new_position = pacman_ghost.position;
		switch(new_direction)
		{
		case GridDirection::XPlus:
		case GridDirection::XMinus:
			new_position[1] = IntToFixed16(Fixed16RoundToInt(new_position[1]));
			break;
		case GridDirection::YPlus:
		case GridDirection::YMinus:
			new_position[0] = IntToFixed16(Fixed16RoundToInt(new_position[0]));
			break;
		}

		// Make sure we still do not move towards other enemy/ghost even after alignemnt to grid.
		if(!movement_is_blocked(new_position))
		{
			pacman_ghost.position = new_position;
			pacman_ghost.direction = new_direction;
		}
	}

	if(player_ != std::nullopt && tick_ >= player_->shield_end_tick)
	{
		const fixed16_t eat_dist = g_tank_half_size;
		if( Fixed16Abs(pacman_ghost.position[0] - player_->position[0]) <= eat_dist &&
			Fixed16Abs(pacman_ghost.position[1] - player_->position[1]) <= eat_dist)
		{
			KillPlayer();

			// Turn around after killing the player.
			GridDirection new_direction = pacman_ghost.direction;
			switch(pacman_ghost.direction)
			{
			case GridDirection::XPlus:
				new_direction = GridDirection::XMinus;
				break;
			case GridDirection::XMinus:
				new_direction = GridDirection::XPlus;
				break;
			case GridDirection::YPlus:
				new_direction = GridDirection::YMinus;
				break;
			case GridDirection::YMinus:
				new_direction = GridDirection::YPlus;
				break;
			}
			pacman_ghost.direction = new_direction;
		}
	}
}

bool GameBattleCity::UpdateProjectile(Projectile& projectile, const bool is_player_projectile)
{
	const fixed16_t speed = projectile.is_armor_piercing ? g_armor_piercing_projectile_speed : g_projectile_speed;
	switch (projectile.direction)
	{
	case GridDirection::XPlus:
		projectile.position[0] += speed;
		break;
	case GridDirection::XMinus:
		projectile.position[0] -= speed;
		break;
	case GridDirection::YPlus:
		projectile.position[1] += speed;
		break;
	case GridDirection::YMinus:
		projectile.position[1] -= speed;
		break;
	}

	const fixed16_t min_x_f = projectile.position[0] - g_projectile_half_size;
	const fixed16_t min_y_f = projectile.position[1] - g_projectile_half_size;
	const fixed16_t max_x_f = projectile.position[0] + g_projectile_half_size;
	const fixed16_t max_y_f = projectile.position[1] + g_projectile_half_size;
	const int32_t min_x = Fixed16FloorToInt(min_x_f);
	const int32_t min_y = Fixed16FloorToInt(min_y_f);
	const int32_t max_x = Fixed16CeilToInt(max_x_f);
	const int32_t max_y = Fixed16CeilToInt(max_y_f);

	if(min_x < 0 || max_x > int32_t(c_field_width ) || min_y < 0 || max_y > int32_t(c_field_height))
	{
		if(is_player_projectile)
		{
			MakeEventSound(SoundId::ArkanoidBallHit);
		}
		MakeExplosion(projectile.position);
		return true;
	}

	for(size_t i = 0; i < enemies_.size() && is_player_projectile; ++i)
	{
		Enemy& enemy = enemies_[i];

		if(tick_ < enemy.spawn_tick + g_enemy_spawn_animation_duration)
		{
			continue;
		}

		const fixed16vec2_t min = {enemy.position[0] - g_tank_half_size, enemy.position[1] - g_tank_half_size};
		const fixed16vec2_t max = {enemy.position[0] + g_tank_half_size, enemy.position[1] + g_tank_half_size};

		if(min[0] >= max_x_f || max[0] <= min_x_f || min[1] >= max_y_f || max[1] <= min_y_f)
		{
			continue;
		}

		// Hit this enemy.
		MakeExplosion(projectile.position);

		assert(enemy.health > 0);
		if(projectile.is_armor_piercing)
		{
			enemy.health = 0;
		}
		else
		{
			--enemy.health;
		}
		if(enemy.health == 0)
		{
			MakeEventSound(SoundId::Explosion);
			MakeExplosion(enemy.position);
			if(enemy.gives_bonus)
			{
				SpawnBonus();
			}
			++extra_elements_spawn_points_;

			if(i + 1 < enemies_.size())
			{
				enemy = std::move(enemies_.back());
			}
			enemies_.pop_back();
		}
		else
		{
			MakeEventSound(SoundId::ProjectileHit);
		}

		return true;
	}
	for(size_t i = 0; i < pacman_ghosts_.size() && is_player_projectile; ++i)
	{
		PacmanGhost& pacman_ghost = pacman_ghosts_[i];

		const fixed16vec2_t min = {pacman_ghost.position[0] - g_tank_half_size, pacman_ghost.position[1] - g_tank_half_size};
		const fixed16vec2_t max = {pacman_ghost.position[0] + g_tank_half_size, pacman_ghost.position[1] + g_tank_half_size};

		if(min[0] >= max_x_f || max[0] <= min_x_f || min[1] >= max_y_f || max[1] <= min_y_f)
		{
			continue;
		}

		// Hit this ghost.
		MakeExplosion(projectile.position);
		MakeEventSound(SoundId::Explosion);

		if(i + 1 < pacman_ghosts_.size())
		{
			pacman_ghost = std::move(pacman_ghosts_.back());
		}
		pacman_ghosts_.pop_back();

		return true;
	}

	if(!is_player_projectile && player_ != std::nullopt)
	{
		for(size_t i = 0; i < player_->projectiles.size(); ++i)
		{
			Projectile& player_projectile =  player_->projectiles[i];
			const fixed16_t other_min_x_f = player_projectile.position[0] - g_projectile_half_size;
			const fixed16_t other_min_y_f = player_projectile.position[1] - g_projectile_half_size;
			const fixed16_t other_max_x_f = player_projectile.position[0] + g_projectile_half_size;
			const fixed16_t other_max_y_f = player_projectile.position[1] + g_projectile_half_size;
			if( other_min_x_f >= max_x_f || other_max_x_f <= min_x_f ||
				other_min_y_f >= max_y_f || other_max_y_f <= min_y_f)
			{
				continue;
			}

			// Hit player projectile.
			// Deestroy both this projectile and player projectile.
			if(i + 1 < player_->projectiles.size())
			{
				player_projectile = player_->projectiles.back();
			}
			player_->projectiles.pop_back();

			return true;
		}

		// Try to kill player.
		const fixed16vec2_t min = {player_->position[0] - g_tank_half_size, player_->position[1] - g_tank_half_size};
		const fixed16vec2_t max = {player_->position[0] + g_tank_half_size, player_->position[1] + g_tank_half_size};
		if(!(min[0] >= max_x_f || max[0] <= min_x_f || min[1] >= max_y_f || max[1] <= min_y_f))
		{
			if(tick_ >= player_->shield_end_tick)
			{
				MakeExplosion(projectile.position);
				KillPlayer();
			}
			return true;
		}
	}

	// TODO - process collisions against player.

	bool hit = false;
	bool something_is_destroyed = false;
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

		if(projectile.is_armor_piercing)
		{
			if(block.type == BlockType::Bricks ||
				block.type == BlockType::Concrete ||
				(block.type >= BlockType::TetrisBlock0 && block.type <= BlockType::TetrisBlock6))
			{
				// Armor-piercing projectile destroys blocks with one shot, even concrete blocks.
				something_is_destroyed = true;
				block.destruction_mask = 0;
			}
		}
		else
		{
			if(block.type == BlockType::Bricks)
			{
				something_is_destroyed = true;

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
			else if(block.type >= BlockType::TetrisBlock0 && block.type <= BlockType::TetrisBlock6)
			{
				something_is_destroyed = true;

				// Destroy tetris blocks with one shot.
				block.destruction_mask = 0;
			}
		}

		// TODO - destroy also concrete blocks if projectile is from upgraded player tank.
		hit = true;
	}

	if(hit)
	{
		if(is_player_projectile)
		{
			MakeEventSound(something_is_destroyed ? SoundId::ProjectileHit : SoundId::ArkanoidBallHit);
		}
		MakeExplosion(projectile.position);
		return true;
	}

	if( !hit && !base_is_destroyed_ &&
		min_x >= int32_t(c_field_width / 2 - 1) && max_x <= int32_t(c_field_width / 2 + 1) &&
		min_y >= int32_t(c_field_height - 2) && max_y <= int32_t(c_field_height))
	{
		MakeEventSound(SoundId::ArkanoidBallHit);
		MakeExplosion(projectile.position);
		MakeExplosion({IntToFixed16(int32_t(c_field_width / 2)), IntToFixed16(int32_t(c_field_height - 1))});
		base_is_destroyed_ = true;
		game_over_ = true;
		return true;
	}

	// TODO - make sound effects on collision.

	return false;
}

bool GameBattleCity::CanMove(const fixed16vec2_t& position) const
{
	const int32_t min_x = Fixed16FloorToInt(position[0] - g_tank_half_size);
	const int32_t min_y = Fixed16FloorToInt(position[1] - g_tank_half_size);
	const int32_t max_x = Fixed16CeilToInt(position[0] + g_tank_half_size);
	const int32_t max_y = Fixed16CeilToInt(position[1] + g_tank_half_size);

	if( min_x < 0 || max_x > int32_t(c_field_width ) ||
		min_y < 0 || max_y > int32_t(c_field_height))
	{
		return false;
	}

	if(!base_is_destroyed_ &&
		!(max_x < int32_t(c_field_width / 2) || min_x > int32_t(c_field_width / 2) ||
		 max_y < int32_t(c_field_height - 1) || min_y > int32_t(c_field_height)))
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

void GameBattleCity::KillPlayer()
{
	MakeExplosion(player_->position);
	MakeEventSound(SoundId::CharacterDeath);
	player_ = std::nullopt;
	player_level_ = 1;
}

void GameBattleCity::SpawnPlayer()
{
	Player player;
	player.position = {IntToFixed16(int32_t(c_field_width / 2 - 4)), IntToFixed16(int32_t(c_field_height - 1))};
	player.direction = GridDirection::YMinus;
	player.shield_end_tick = tick_ + g_spawn_shield_duration;

	player_ = player;
}

void GameBattleCity::SpawnNewEnemy()
{
	assert(enemies_left_ > 0);

	// Try to spawn it at random position, but avoid obstacles.
	for(uint32_t i = 0; i < 64; ++i)
	{
		const uint32_t x = 1 + (rand_.Next() % (c_field_width - 2));
		const uint32_t y = 1;

		const fixed16vec2_t position = {IntToFixed16(int32_t(x)), IntToFixed16(int32_t(y))};

		if(!CanMove(position))
		{
			continue;
		}

		bool position_blocked = false;
		for(const Enemy& enemy : enemies_)
		{
			if(TanksIntersects(position, enemy.position))
			{
				position_blocked = true;
				break;
			}
		}
		for(const PacmanGhost& pacman_ghost : pacman_ghosts_)
		{
			if(TanksIntersects(position, pacman_ghost.position))
			{
				position_blocked = true;
				break;
			}
		}
		if(player_ != std::nullopt)
		{
			position_blocked |= TanksIntersects(position, player_->position);
		}

		if(position_blocked)
		{
			continue;
		}

		Enemy enemy;
		enemy.type = EnemyType(rand_.Next() % uint32_t(EnemyType::NumTypes));
		enemy.health = enemy.type == EnemyType::Heavy ? 3 : 1;
		enemy.position = position;
		enemy.direction = GridDirection::YPlus;
		enemy.spawn_tick = tick_;

		bool have_bonus_enemy = false;
		for(const Enemy& enemy : enemies_)
		{
			have_bonus_enemy |= enemy.gives_bonus;
		}

		if(!have_bonus_enemy)
		{
			enemy.gives_bonus = rand_.Next() % 4 == 2;
		}

		enemies_.push_back(enemy);

		--enemies_left_;
		return;
	}
}

void GameBattleCity::SpawnPacmanGhost()
{
	// Try to spawn it at random position, but avoid obstacles.
	for(uint32_t i = 0; i < 64; ++i)
	{
		const uint32_t x = 1 + (rand_.Next() % (c_field_width - 2));
		const uint32_t y = 1;

		const fixed16vec2_t position = {IntToFixed16(int32_t(x)), IntToFixed16(int32_t(y))};

		if(!CanMove(position))
		{
			continue;
		}

		bool position_blocked = false;
		for(const Enemy& enemy : enemies_)
		{
			if(TanksIntersects(position, enemy.position))
			{
				position_blocked = true;
				break;
			}
		}
		for(const PacmanGhost& pacman_ghost : pacman_ghosts_)
		{
			if(TanksIntersects(position, pacman_ghost.position))
			{
				position_blocked = true;
				break;
			}
		}
		if(player_ != std::nullopt)
		{
			position_blocked |= TanksIntersects(position, player_->position);
		}

		if(position_blocked)
		{
			continue;
		}

		PacmanGhostType ghost_type = PacmanGhostType::Blinky;
		for(uint32_t j = 0; j < 64; ++j)
		{
			ghost_type = PacmanGhostType(rand_.Next() % 4);
			bool is_unique = true;
			for(const PacmanGhost& pacman_ghost : pacman_ghosts_)
			{
				is_unique &= pacman_ghost.type != ghost_type;
			}

			if(is_unique)
			{
				break;
			}
		}

		PacmanGhost pacman_ghost;
		pacman_ghost.position = position;
		pacman_ghost.direction = GridDirection::YPlus;
		pacman_ghost.type = ghost_type;

		pacman_ghosts_.push_back(pacman_ghost);
		return;
	}
}

void GameBattleCity::TrySpawnExtraElement()
{
	// Give spawn points for enemies kill. Make extra elements spawn dependent on number of killed enemies, not just time.

	if(extra_elements_spawn_points_ == 0)
	{
		return;
	}
	if(snake_bonus_ != std::nullopt && pacman_ghosts_.size() >= g_max_alive_pacman_ghosts)
	{
		return;
	}

	if(rand_.Next() % 745 != 29)
	{
		return;
	}

	--extra_elements_spawn_points_;
	if(rand_.Next() % 10 >= 6)
	{
		// Spawn something not for each spawn point.
		return;
	}

	if(pacman_ghosts_.size() >= g_max_alive_pacman_ghosts)
	{
		SpawnSnakeBonus();
	}
	else if(snake_bonus_ != std::nullopt)
	{
		SpawnPacmanGhost();
	}
	else
	{
		if(rand_.Next() % 3 == 0)
		{
			SpawnPacmanGhost();
		}
		else
		{
			SpawnSnakeBonus();
		}
	}
}

void GameBattleCity::SpawnBonus()
{
	const auto bonus_type = BonusType(rand_.Next() % uint32_t(BonusType::NumTypes));
	for(uint32_t i = 0; i < 256; ++i)
	{
		const uint32_t x = rand_.Next() % (c_field_width  - 2) + 1;
		const uint32_t y = rand_.Next() % (c_field_height - 4) + 1;

		bool can_place = true;
		for(uint32_t dy = 0; dy < 2; ++dy)
		for(uint32_t dx = 0; dx < 2; ++dx)
		{
			const BlockType block_type = field_[x + dx - 1 + (y + dy - 1) * c_field_width].type;
			can_place &= block_type ==
				BlockType::Empty || block_type == BlockType::Bricks || block_type == BlockType::Foliage;
		}
		if(!can_place)
		{
			continue;
		}

		const fixed16vec2_t position = {IntToFixed16(int32_t(x)), IntToFixed16(int32_t(y))};
		if(player_ != std::nullopt && TanksIntersects(position, player_->position))
		{
			// Do not spawn bonus directly at player position.
			continue;
		}

		Bonus bonus;
		bonus.type = bonus_type;
		bonus.position = position;
		bonus_ = bonus;
		return;
	}
}

void GameBattleCity::SpawnSnakeBonus()
{
	if(snake_bonus_ != std::nullopt)
	{
		return;
	}

	const auto bonus_type = SnakeBonusType(rand_.Next() % uint32_t(SnakeBonusType::NumTypes));
	for(uint32_t i = 0; i < 256; ++i)
	{
		const uint32_t x = rand_.Next() % (c_field_width  - 2) + 1;
		const uint32_t y = rand_.Next() % (c_field_height - 4) + 1;

		const Block& block = field_[x + y * c_field_width];
		if(!(block.type == BlockType::Empty || block.destruction_mask == 0))
		{
			continue;
		}

		const fixed16vec2_t position =
			{IntToFixed16(int32_t(x)) + g_fixed16_one / 2, IntToFixed16(int32_t(y)) + g_fixed16_one / 2};
		if(player_ != std::nullopt && TanksIntersects(position, player_->position))
		{
			// Do not spawn bonus directly at player position.
			continue;
		}

		SnakeBonus bonus;
		bonus.type = bonus_type;
		bonus.position = position;
		snake_bonus_ = bonus;
		return;
	}
}

void GameBattleCity::MakeExplosion(const fixed16vec2_t& position)
{
	Explosion explosion;
	explosion.position = position;
	explosion.start_tick = tick_;

	explosions_.push_back(explosion);
}

void GameBattleCity::ActivateBaseProtectionBonus()
{
	base_protection_bonus_end_tick_ = tick_ + g_base_protection_bonus_duration;

	for(const auto& tile : c_base_wall_tiles)
	{
		Block& block = field_[tile[0] + tile[1] * c_field_width];
		block.type = BlockType::Concrete;
		block.destruction_mask = 0xF;
	}
}

void GameBattleCity::UpdateBaseProtectionBonus()
{
	if(tick_ == base_protection_bonus_end_tick_)
	{
		for(const auto& tile : c_base_wall_tiles)
		{
			field_[tile[0] + tile[1] * c_field_width].type = BlockType::Bricks;
		}
	}
}

void GameBattleCity::BlockEnemiesWithTetrisFigures()
{
	for(const Enemy& enemy : enemies_)
	{
		BlockEnemyWithTetrisFigure(enemy.position);
	}

	// Do not block packman ghosts becase ghost cant' shoot and can't free themselves from blocking.
}

void GameBattleCity::BlockEnemyWithTetrisFigure(const fixed16vec2_t& position)
{
	const int32_t enemy_min_x = Fixed16FloorToInt(position[0] - g_tank_half_size) - 1;
	const int32_t enemy_min_y = Fixed16FloorToInt(position[1] - g_tank_half_size) - 1;
	const int32_t enemy_max_x = Fixed16FloorToInt(position[0] + g_tank_half_size) + 1;
	const int32_t enemy_max_y = Fixed16FloorToInt(position[1] + g_tank_half_size) + 1;

	/* Try to place tetris figures at free space touching these cells.
	  ####
	  #  #
	  #  #
	  ####
	*/

	for(int32_t i = 0; i < 256; ++i)
	{
		const uint32_t type_index = rand_.Next() % g_tetris_num_piece_types;
		const TetrisBlock type = TetrisBlock(uint32_t(TetrisBlock::I) + type_index);

		TetrisPieceBlocks blocks = g_tetris_pieces_blocks[type_index];

		// Choose random rotation.
		const uint32_t num_rotations = rand_.Next() / 47u % 4u;
		for(uint32_t i = 0; i < num_rotations; ++i)
		{
			TetrisPiece piece;
			piece.type = type;
			piece.blocks = blocks;
			blocks = RotateTetrisPieceBlocks(piece);
		}

		int32_t figure_min_x = 99999, figure_max_x = -9999;
		int32_t figure_min_y = 99999, figure_max_y = -9999;
		for(const TetrisPieceBlock& block : blocks)
		{
			figure_min_x = std::min(figure_min_x, block[0]);
			figure_max_x = std::max(figure_max_x, block[0]);
			figure_min_y = std::min(figure_min_y, block[1]);
			figure_max_y = std::max(figure_max_y, block[1]);
		}

		const int32_t offset_min_x = enemy_min_x - figure_max_x;
		const int32_t offset_min_y = enemy_min_y - figure_max_y;
		const int32_t offset_max_x = enemy_max_x - figure_min_x;
		const int32_t offset_max_y = enemy_max_y - figure_min_y;

		assert(offset_min_x <= offset_max_x);
		assert(offset_min_y <= offset_max_y);

		const int32_t offset_x = offset_min_x + int32_t(rand_.Next() % uint32_t(1 + offset_max_x - offset_min_x));
		const int32_t offset_y = offset_min_y + int32_t(rand_.Next() % uint32_t(1 + offset_max_y - offset_min_y));

		TetrisPieceBlocks blocks_shifted = blocks;
		for(TetrisPieceBlock& block_shifted : blocks_shifted)
		{
			block_shifted[0] += offset_x;
			block_shifted[1] += offset_y;
		}

		bool can_place = true;
		for(const TetrisPieceBlock& block_shifted : blocks_shifted)
		{
			if( block_shifted[0] >= 0 && block_shifted[0] < int32_t(c_field_width ) &&
				block_shifted[1] >= 0 && block_shifted[1] < int32_t(c_field_height))
			{
				const Block& field_block = field_[uint32_t(block_shifted[0]) + uint32_t(block_shifted[1]) * c_field_width];
				can_place &= field_block.type == BlockType::Empty || field_block.destruction_mask == 0;
			}
			else
			{
				can_place = false;
				break;
			}

			const fixed16vec2_t bbox_min{IntToFixed16(block_shifted[0]), IntToFixed16(block_shifted[1])};
			const fixed16vec2_t bbox_max{bbox_min[0] + g_fixed16_one, bbox_min[1] + g_fixed16_one};

			for(const Enemy& enemy : enemies_)
			{
				can_place &=
					!BBoxesIntersect(
						bbox_min,
						bbox_max,
						{enemy.position[0] - g_tank_half_size, enemy.position[1] - g_tank_half_size},
						{enemy.position[0] + g_tank_half_size, enemy.position[1] + g_tank_half_size});
			}
			for(const PacmanGhost& pacman_ghost : pacman_ghosts_)
			{
				can_place &=
					!BBoxesIntersect(
						bbox_min,
						bbox_max,
						{pacman_ghost.position[0] - g_tank_half_size, pacman_ghost.position[1] - g_tank_half_size},
						{pacman_ghost.position[0] + g_tank_half_size, pacman_ghost.position[1] + g_tank_half_size});
			}
			if(player_ != std::nullopt)
			{
				can_place &=
					!BBoxesIntersect(
						bbox_min,
						bbox_max,
						{player_->position[0] - g_tank_half_size, player_->position[1] - g_tank_half_size},
						{player_->position[0] + g_tank_half_size, player_->position[1] + g_tank_half_size});
			}
		}

		if(!can_place)
		{
			continue;
		}

		// Place it.

		for(const TetrisPieceBlock& block_shifted : blocks_shifted)
		{
			Block& field_block = field_[uint32_t(block_shifted[0]) + uint32_t(block_shifted[1]) * c_field_width];
			field_block.type = BlockType(uint32_t(BlockType::TetrisBlock0) + type_index);
			field_block.destruction_mask = 0xF;
		}
	} // for tries.
}

void GameBattleCity::MakeEventSound(const SoundId sound_id)
{
	sound_player_.PlaySound(sound_id);

	ActiveSound active_sound;
	active_sound.id = sound_id;
	active_sound.end_tick = tick_ + GameInterface::c_update_frequency / 2;
	current_sound_ = active_sound;
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

GameBattleCity::Projectile GameBattleCity::MakeProjectile(
	const fixed16vec2_t& tank_position,
	const GridDirection tank_direction,
	const bool is_armor_piercing)
{
	Projectile projectile;
	projectile.position = tank_position;
	projectile.direction = tank_direction;
	projectile.is_armor_piercing = is_armor_piercing;

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

	return projectile;
}
