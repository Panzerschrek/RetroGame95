#include "GameArkanoid.hpp"
#include "ArkanoidLevels.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "GameTetris.hpp"
#include "Sprites.hpp"
#include <cassert>
#include <cmath>

namespace
{

const fixed16_t g_ball_base_speed = g_fixed16_one * 5 / 4;
const fixed16_t g_bonus_drop_speed = g_fixed16_one / 2;
const fixed16_t g_laser_beam_speed = g_fixed16_one * 2;

const uint32_t g_bonus_drop_inv_chance = 7;
const uint32_t g_max_lifes = 6;
const uint32_t g_ship_modifier_bonus_duration = 500;
const uint32_t g_slow_down_bonus_duration = 960;
const uint32_t g_death_animation_duration = 120;
const uint32_t g_death_animation_flicker_duration = 12;
const uint32_t g_level_start_animation_duration = 360;
const uint32_t g_min_shoot_interval = 45;

} // namespace

GameArkanoid::GameArkanoid(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
	NextLevel();
}

void GameArkanoid::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	++tick_;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
	}

	if(tick_ >= level_start_animation_end_tick_)
	{
		ProcessLogic(events, keyboard_state);
	}
}

void GameArkanoid::Draw(const FrameBuffer frame_buffer) const
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_offset_x = 10;
	const uint32_t field_offset_y = 10;

	const uint32_t texts_offset_x = 256;
	const uint32_t texts_offset_y = 40;

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

	for(uint32_t y = 0; y < c_field_height; ++y)
	{
		for(uint32_t x = 0; x < c_field_width; ++x)
		{
			const Block& block = field_[x + y * c_field_width];
			if(block.type == BlockType::Empty)
			{
				continue;
			}
			DrawSpriteWithAlpha(
				frame_buffer,
				block_sprites[uint32_t(block.type) - 1],
				0,
				field_offset_x + x * c_block_width,
				field_offset_y + y * c_block_height);
		}
	}

	const bool playing_level_start_animation = tick_ < level_start_animation_end_tick_;

	if(ship_ != std::nullopt && !playing_level_start_animation)
	{
		SpriteBMP sprite = Sprites::arkanoid_ship;
		switch(ship_->state)
		{
		case ShipState::Normal:
		case ShipState::Sticky:
			sprite = Sprites::arkanoid_ship;
			break;
		case ShipState::Large:
			sprite = Sprites::arkanoid_ship_large;
			break;
		case ShipState::Turret:
			sprite = Sprites::arkanoid_ship_with_turrets;
			break;
		}

		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(ship_->position[0])) - GetShipHalfWidthForState(ship_->state),
			field_offset_y + uint32_t(Fixed16FloorToInt(ship_->position[1])) - c_ship_half_height);
	}
	if(death_animation_ != std::nullopt)
	{
		if((tick_ / g_death_animation_flicker_duration) % 2 == 0)
		{
			DrawSpriteWithAlpha(
				frame_buffer,
				Sprites::arkanoid_ship,
				0,
				field_offset_x + uint32_t(Fixed16FloorToInt(ship_->position[0])) - GetShipHalfWidthForState(ship_->state),
				field_offset_y + uint32_t(Fixed16FloorToInt(ship_->position[1])) - c_ship_half_height);
		}
	}

	for(uint32_t i = 0, ship_life_x = 0; i < lifes_; ++i)
	{
		const uint32_t padding = 3;
		const SpriteBMP sprite(Sprites::arkanoid_ship_life);
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			padding + field_offset_x + ship_life_x,
			padding + field_offset_y + c_block_height * (c_field_height + 1));
		ship_life_x += sprite.GetWidth() + padding;
	}

	if(!playing_level_start_animation)
	{
		for(const Ball& ball : balls_)
		{
			fixed16vec2_t position = ball.position;
			if(ball.is_attached_to_ship)
			{
				if(ship_ == std::nullopt)
				{
					continue;
				}
				position[0] += ship_->position[0];
				position[1] += ship_->position[1];
			}

			DrawSpriteWithAlpha(
				frame_buffer,
				Sprites::arkanoid_ball,
				0,
				field_offset_x + uint32_t(Fixed16FloorToInt(position[0])) - c_ball_half_size,
				field_offset_y + uint32_t(Fixed16FloorToInt(position[1])) - c_ball_half_size);
		}
	}

	for(const LaserBeam& laser_beam : laser_beams_)
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			Sprites::arkanoid_laser_beam,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(laser_beam.position[0])),
			field_offset_y + uint32_t(Fixed16FloorToInt(laser_beam.position[1])) - (c_laser_beam_height + 1) / 2);
	}

	const SpriteBMP bonuses_sprites[]
	{
		Sprites::arkanoid_bonus_b,
		Sprites::arkanoid_bonus_c,
		Sprites::arkanoid_bonus_d,
		Sprites::arkanoid_bonus_e,
		Sprites::arkanoid_bonus_l,
		Sprites::arkanoid_bonus_p,
		Sprites::arkanoid_bonus_s,
	};

	for(const Bonus& bonus : bonuses_)
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			bonuses_sprites[size_t(bonus.type)],
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(bonus.position[0])) - c_bonus_half_width,
			field_offset_y + uint32_t(Fixed16FloorToInt(bonus.position[1])) - c_bonus_half_height);
	}

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

	uint32_t trim_top_x = field_offset_x - 10;
	for(const SpriteBMP& sprite : sprites_trim_top)
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			trim_top_x,
			field_offset_y - 10);

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

	uint32_t trim_side_y = field_offset_y;
	const uint32_t side_trim_offset_x = field_offset_x - 10;
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
			side_trim_offset_x + c_block_width * c_field_width + sprite.GetWidth(),
			trim_side_y);

		trim_side_y += sprite.GetHeight();
	}

	if(next_level_exit_is_open_)
	{
		const SpriteBMP sprite(Sprites::arkanoid_level_exit_gate);
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			side_trim_offset_x + c_block_width * c_field_width + sprite.GetWidth(),
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

		if(!next_level_exit_is_open_)
		{
			DrawSpriteWithAlpha(
				frame_buffer,
				sprite,
				0,
				side_trim_offset_x + c_block_width * c_field_width + sprite.GetWidth(),
				trim_side_y);
		}

		trim_side_y += sprite.GetHeight();
	}

	char text[64];
	std::snprintf(text, sizeof(text), "Round\n\n%3d", level_);
	DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y, text);

	if(tick_ < level_start_animation_end_tick_)
	{
		std::snprintf(text, sizeof(text), "Round %d", level_);

		DrawText(
			frame_buffer,
			g_color_white,
			field_offset_x + c_block_width  * c_field_width  / 2 - 24,
			field_offset_y + c_block_height * c_field_height / 2 - 4,
			text);
	}

	std::snprintf(text, sizeof(text), "Score\n\n%5d", score_);
	DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y + 64, text);

	if(game_over_)
	{
		DrawText(
			frame_buffer,
			g_color_white,
			field_offset_x + c_block_width  * c_field_width  / 2 - 36,
			field_offset_y + c_block_height * c_field_height / 2 - 4,
			"Game Over");
	}
}

GameInterfacePtr GameArkanoid::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameArkanoid::ProcessLogic(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void)keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_MOUSEMOTION)
		{
			if (ship_ != std::nullopt)
			{
				const fixed16_t sensetivity =g_fixed16_one / 3; // TODO - make this configurable.
				ship_->position[0] += event.motion.xrel * sensetivity;
				CorrectShipPosition();
			}
		}
		if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			if(event.button.button == 1)
			{
				ReleaseStickyBalls();
				if(ship_ != std::nullopt && ship_->state == ShipState::Sticky)
				{
					// Reset sticky state after first shot.
					ship_->state = ShipState::Normal;
				}

				if(ship_ != std::nullopt && ship_->state == ShipState::Turret)
				{
					if(tick_ >= ship_->next_shoot_tick)
					{
						const fixed16_t x_delta =
								IntToFixed16(int32_t(GetShipHalfWidthForState(ship_->state) - 5)) - g_fixed16_one / 2;

						LaserBeam beam0;
						beam0.position = ship_->position;
						beam0.position[0] += x_delta;

						LaserBeam beam1;
						beam1.position = ship_->position;
						beam1.position[0] -= x_delta;

						laser_beams_.push_back(beam0);
						laser_beams_.push_back(beam1);

						ship_->next_shoot_tick = tick_ + g_min_shoot_interval;
					}
				}
			}
		}
	}

	if(ship_ != std::nullopt)
	{
		if(ship_->state_end_tick <= tick_)
		{
			// Transition to normal state.
			ReleaseStickyBalls();

			ship_->state = ShipState::Normal;
		}
	}

	for(size_t b = 0; b < balls_.size();)
	{
		if(UpdateBall(balls_[b]))
		{
			// This ball is dead.
			if(b + 1 < balls_.size())
			{
				balls_[b] = balls_.back();
			}
			balls_.pop_back();
		}
		else
		{
			++b;
		}
	} // for balls.

	for(size_t b = 0; b < bonuses_.size();)
	{
		if(UpdateBonus(bonuses_[b]))
		{
			// This bonus is dead.
			if(b + 1 < bonuses_.size())
			{
				bonuses_[b] = bonuses_.back();
			}
			bonuses_.pop_back();
		}
		else
		{
			++b;
		}
	} // for bonuses.

	for(size_t b = 0; b < laser_beams_.size();)
	{
		if(UpdateLaserBeam(laser_beams_[b]))
		{
			// This laser beam is dead.
			if(b + 1 < laser_beams_.size())
			{
				laser_beams_[b] = laser_beams_.back();
			}
			laser_beams_.pop_back();
		}
		else
		{
			++b;
		}
	} // for laser beams.

	if(ship_ != std::nullopt && balls_.empty() && !next_level_exit_is_open_ && death_animation_ == std::nullopt)
	{
		// Lost all balls - kill the ship.

		DeathAnimation death_animation;
		death_animation.ship_position = ship_->position;
		death_animation.end_tick = tick_ + g_death_animation_duration;

		death_animation_ = death_animation;

		ship_ = std::nullopt;
	}
	if(death_animation_ != std::nullopt)
	{
		if(death_animation_->end_tick <= tick_)
		{
			if(lifes_ > 0)
			{
				--lifes_;
				SpawnShip();
			}
			else
			{
				game_over_ = true;
			}

			death_animation_ = std::nullopt;
		}
	}

	// Check for next level exit open condition.
	uint32_t num_non_empty_blocks = 0;
	for(uint32_t y = 0; y < c_field_height; ++y)
	{
		for(uint32_t x = 0; x < c_field_width; ++x)
		{
			const Block& block = field_[x + y * c_field_width];
			if(block.type != BlockType::Empty)
			{
				++num_non_empty_blocks;
			}
		}
	}

	// Open next level exit if all blocks are destroyed.
	if(num_non_empty_blocks == 0)
	{
		next_level_exit_is_open_ = true;
	}

	// Transition to next level when ship reaches exit.
	if(next_level_exit_is_open_ &&
		ship_ != std::nullopt && ship_->position[0] >= IntToFixed16(c_field_width * c_block_width))
	{
		if(level_ < std::size(arkanoid_levels))
		{
			NextLevel();
		}
		else
		{
			next_game_ = std::make_unique<GameTetris>(sound_player_);
		}
	}
}

void GameArkanoid::NextLevel()
{
	++level_;

	if(lifes_ < 3)
	{
		lifes_ += 1;
	}

	balls_.clear();
	bonuses_.clear();
	laser_beams_.clear();
	next_level_exit_is_open_ = false;
	slow_down_end_tick_ = 0;

	const char* level_data = arkanoid_levels[(level_ - 1) % std::size(arkanoid_levels)];
	for(uint32_t y = 0; y < c_field_height; ++y)
	{
		for(uint32_t x = 0; x < c_field_width; ++x, ++level_data)
		{
			Block& block = field_[x + y * c_field_width];
			block.type = GetBlockTypeForLevelDataByte(*level_data);

			block.health = 1;
			if(block.type == BlockType::Concrete)
			{
				block.health = 2;
			}
			else if(block.type == BlockType::Color14_15)
			{
				block.health = 4;
			}
		}
		assert(*level_data == '\n');
		++level_data;
	}

	SpawnShip();

	level_start_animation_end_tick_ = tick_ + g_level_start_animation_duration;
}

void GameArkanoid::SpawnShip()
{
	balls_.clear();

	Ball ball;
	ball.position =
	{
		0,
		-IntToFixed16(c_ship_half_height + c_ball_half_size),
	};
	ball.is_attached_to_ship = true;
	ball.velocity = { 0, -g_ball_base_speed };
	balls_.push_back(ball);

	Ship ship;
	ship.position =
	{
		IntToFixed16(c_field_width * c_block_width / 2),
		IntToFixed16(c_field_height * c_block_height + c_ship_half_height),
	};

	ship.state = ShipState::Sticky;
	ship.state_end_tick = tick_ + g_ship_modifier_bonus_duration;

	ship_ = ship;
}

bool GameArkanoid::UpdateBall(Ball& ball)
{
	if(ball.is_attached_to_ship)
	{
		// Just to make sure we do not have attached ball without a ship - kill ball if ship is destroyed.
		return ship_ == std::nullopt;
	}

	if(slow_down_end_tick_ > tick_)
	{
		ball.position[0] += ball.velocity[0] / 2;
		ball.position[1] += ball.velocity[1] / 2;
	}
	else
	{
		ball.position[0] += ball.velocity[0];
		ball.position[1] += ball.velocity[1];
	}

	const fixed16_t ball_half_size = IntToFixed16(c_ball_half_size);

	// Bounce ball from blocks.
	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width; ++x)
	{
		Block& block = field_[x + y * c_field_width];
		if(block.type == BlockType::Empty)
		{
			continue;
		}

		// Replace box<-> box collision with extended box<->point collision.

		const fixed16vec2_t borders_min =
		{
			IntToFixed16(int32_t(x * c_block_width) ) - ball_half_size,
			IntToFixed16(int32_t(y * c_block_height)) - ball_half_size,
		};
		const fixed16vec2_t borders_max =
		{
			IntToFixed16(int32_t((x + 1) * c_block_width )) + ball_half_size,
			IntToFixed16(int32_t((y + 1) * c_block_height)) + ball_half_size,
		};

		if( ball.position[0] <= borders_min[0] || ball.position[0] >= borders_max[0] ||
			ball.position[1] <= borders_min[1] || ball.position[1] >= borders_max[1])
		{
			continue;
		}

		// Hit this block.
		DamageBlock(x, y);
		sound_player_.PlaySound(SoundId::ArkanoidBallHit);

		// Ball intersectss with this block. Try to push it.
		// Find closest intersection of negative velocity vector and extended block side in order to do this.

		// TODO - extract this code into separate function.

		int64_t closest_square_dist = 0x7FFFFFFFFFFFFFFF;
		fixed16vec2_t closest_position = ball.position;
		std::array<int32_t, 2> bounce_vec = {0, 0};
		const fixed16_t vel_div_clamp = g_fixed16_one / 256; // Avoid overflow in division.
		if(ball.velocity[0] > 0)
		{
			const fixed16vec2_t intersection_pos
			{
				borders_min[0],
				ball.position[1] -
					Fixed16MulDiv(
						ball.position[0] - borders_min[0],
						ball.velocity[1],
						std::max(ball.velocity[0], vel_div_clamp)),
			};
			const fixed16vec2_t vec_to_intersection_pos
			{
				ball.position[0] - intersection_pos[0],
				ball.position[1] - intersection_pos[1]
			};
			const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
			if(square_dist < closest_square_dist)
			{
				closest_square_dist = square_dist;
				closest_position = intersection_pos;
				bounce_vec = {1, 0};
			}
		}
		else if(ball.velocity[0] < 0)
		{
			const fixed16vec2_t intersection_pos
			{
				borders_max[0],
				ball.position[1] +
					Fixed16MulDiv(
						borders_max[0] - ball.position[0],
						ball.velocity[1],
						std::min(ball.velocity[0], -vel_div_clamp)),
			};
			const fixed16vec2_t vec_to_intersection_pos
			{
				ball.position[0] - intersection_pos[0],
				ball.position[1] - intersection_pos[1]
			};
			const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
			if(square_dist < closest_square_dist)
			{
				closest_square_dist = square_dist;
				closest_position = intersection_pos;
				bounce_vec = {1, 0};
			}
		}

		if(ball.velocity[1] > 0)
		{
			const fixed16vec2_t intersection_pos
			{
				ball.position[0] -
					Fixed16MulDiv(
						ball.position[1] - borders_min[1],
						ball.velocity[0],
						std::max(ball.velocity[1], vel_div_clamp)),
				borders_min[1],
			};
			const fixed16vec2_t vec_to_intersection_pos
			{
				ball.position[0] - intersection_pos[0],
				ball.position[1] - intersection_pos[1]
			};
			const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
			if(square_dist < closest_square_dist)
			{
				closest_square_dist = square_dist;
				closest_position = intersection_pos;
				bounce_vec = {0, 1};
			}
		}
		else if(ball.velocity[1] < 0)
		{
			const fixed16vec2_t intersection_pos
			{
				ball.position[0] +
					Fixed16MulDiv(
						borders_max[1] - ball.position[1],
						ball.velocity[0],
						std::min(ball.velocity[1], -vel_div_clamp)),
				borders_max[1],
			};
			const fixed16vec2_t vec_to_intersection_pos
			{
				ball.position[0] - intersection_pos[0],
				ball.position[1] - intersection_pos[1]
			};
			const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
			if(square_dist < closest_square_dist)
			{
				closest_square_dist = square_dist;
				closest_position = intersection_pos;
				bounce_vec = {0, 1};
			}
		}

		if(bounce_vec[0] != 0)
		{
			ball.position[0] = 2 * closest_position[0] - ball.position[0];
			ball.velocity[0] = -ball.velocity[0];
		}
		if(bounce_vec[1] != 0)
		{
			ball.position[1] = 2 * closest_position[1] - ball.position[1];
			ball.velocity[1] = -ball.velocity[1];
		}
	} // for blocks.

	if(ship_ != std::nullopt)
	{
		const fixed16_t half_width_extended =
			IntToFixed16(int32_t(GetShipHalfWidthForState(ship_->state))) + ball_half_size;
		const fixed16_t ship_upper_border_extended =
			ship_->position[1] - IntToFixed16(c_ship_half_height) - ball_half_size;
		if((ball.position[0] >= ship_->position[0] - half_width_extended &&
			ball.position[0] <= ship_->position[0] + half_width_extended) &&
			ball.position[1] >= ship_upper_border_extended)
		{
			// Bounce ball from the ship.
			ball.position[1] = 2 * ship_upper_border_extended - ball.position[1];
			assert(ball.position[1] <= ship_upper_border_extended);

			// Calculate velocity, based on hit position and ball speed.

			// Value in range close to [-1; 1].
			const fixed16_t relative_position =
				Fixed16Div(ball.position[0] - ship_->position[0], half_width_extended);

			const fixed16_t cos_45_deg = 46341;
			const fixed16_t angle_cos = Fixed16Mul(relative_position, cos_45_deg);
			// TODO - use integer square root instead.
			const fixed16_t angle_sin =
				fixed16_t(
					std::sqrt(
						std::max(
							float(g_fixed16_one) * float(g_fixed16_one) - float(angle_cos) * float(angle_cos),
							0.0f)));

			ball.velocity[0] = Fixed16Mul(g_ball_base_speed, angle_cos);
			ball.velocity[1] = -Fixed16Mul(g_ball_base_speed, angle_sin);

			if(ship_->state == ShipState::Sticky)
			{
				ball.position[1] = ship_upper_border_extended;
				ball.position[0] -= ship_->position[0];
				ball.position[1] -= ship_->position[1];
				ball.is_attached_to_ship = true;

				return false;
			}
		}
	}

	// Bounce ball from walls.
	// Do this only after blocks and ship bouncing to make sure that ball is inside game field.
	const fixed16vec2_t filed_mins =
	{
		ball_half_size,
		ball_half_size,
	};
	const fixed16vec2_t filed_maxs =
	{
		IntToFixed16(c_field_width * c_block_width) - ball_half_size,
		 // Increase lower border to disable floor bounce.
		IntToFixed16((c_field_height + 10) * c_block_height) - ball_half_size,
	};

	for(size_t i = 0; i < 2; ++i)
	{
		if(ball.position[i] < filed_mins[i])
		{
			ball.velocity[i] = -ball.velocity[i];
			ball.position[i] = 2 * filed_mins[i] - ball.position[i];
			assert(ball.position[i] >= filed_mins[i]);
		}
		if(ball.position[i] > filed_maxs[i])
		{
			ball.velocity[i] = -ball.velocity[i];
			ball.position[i] = 2 * filed_maxs[i] - ball.position[i];
			assert(ball.position[i] <= filed_maxs[i]);
		}
	}

	// Kill the ball if it reaches lower field border.
	return ball.position[1] > IntToFixed16(c_field_height * c_block_height);
}

bool GameArkanoid::UpdateBonus(Bonus& bonus)
{
	bonus.position[1] += g_bonus_drop_speed;

	if(ship_ != std::nullopt)
	{
		const fixed16_t half_width_extended =
			IntToFixed16(int32_t(GetShipHalfWidthForState(ship_->state) + c_bonus_half_width));
		const fixed16_t ship_upper_border_extended =
			ship_->position[1] - IntToFixed16(c_ship_half_height + c_bonus_half_height);
		if((bonus.position[0] >= ship_->position[0] - half_width_extended &&
			bonus.position[0] <= ship_->position[0] + half_width_extended) &&
			bonus.position[1] >= ship_upper_border_extended)
		{
			// This bonus is cathed.

			// Add score for bonus catch.
			score_ += 48;

			switch(bonus.type)
			{
			case BonusType::NextLevel:
				next_level_exit_is_open_ = true;
				break;

			case BonusType::BallSplit:
				SplitBalls();
				break;

			case BonusType::StickyShip:
				ReleaseStickyBalls();
				ship_->state = ShipState::Sticky;
				ship_->state_end_tick = tick_ + g_ship_modifier_bonus_duration;
				CorrectShipPosition();
				break;

			case BonusType::LargeShip:
				ReleaseStickyBalls();
				ship_->state = ShipState::Large;
				ship_->state_end_tick = tick_ + g_ship_modifier_bonus_duration;
				CorrectShipPosition();
				break;

			case BonusType::LaserShip:
				ReleaseStickyBalls();
				ship_->state = ShipState::Turret;
				ship_->state_end_tick = tick_ + g_ship_modifier_bonus_duration;
				CorrectShipPosition();
				break;

			case BonusType::ExtraLife:
				lifes_ = std::min(lifes_ + 1, g_max_lifes);
				break;

			case BonusType::SlowDown:
				slow_down_end_tick_ = tick_ + g_slow_down_bonus_duration;
				break;

			case BonusType::NumBonuses:
				assert(false);
				break;
			}

			return true;
		}
	}

	// Kill the bonus if it reaches lower field border.
	return bonus.position[1] > IntToFixed16(c_field_height * c_block_height);
}

bool GameArkanoid::UpdateLaserBeam(LaserBeam& laser_beam)
{
	laser_beam.position[1] -= g_laser_beam_speed;

	const fixed16_t beam_half_height = IntToFixed16(c_laser_beam_height) / 2;

	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width; ++x)
	{
		Block& block = field_[x + y * c_field_width];
		if(block.type == BlockType::Empty)
		{
			continue;
		}

		const fixed16vec2_t borders_min =
		{
			IntToFixed16(int32_t(x * c_block_width) ),
			IntToFixed16(int32_t(y * c_block_height)) - beam_half_height,
		};
		const fixed16vec2_t borders_max =
		{
			IntToFixed16(int32_t((x + 1) * c_block_width )),
			IntToFixed16(int32_t((y + 1) * c_block_height)) + beam_half_height,
		};

		if( laser_beam.position[0] <= borders_min[0] || laser_beam.position[0] >= borders_max[0] ||
			laser_beam.position[1] <= borders_min[1] || laser_beam.position[1] >= borders_max[1])
		{
			continue;
		}

		// Hit the block.
		DamageBlock(x, y);

		// Destroy laser beam at first hit.
		return true;
	}

	return laser_beam.position[1] <= 0;
}

void GameArkanoid::DamageBlock(const uint32_t block_x, const uint32_t block_y)
{
	Block& block = field_[ block_x + block_y * c_field_width ];
	if(block.type == BlockType::Empty)
	{
		return;
	}

	// TODO - tune score calculation.
	score_ += 16;

	assert(block.health > 0);
	--block.health;

	if(block.health == 0)
	{
		block.type = BlockType::Empty;
		TrySpawnNewBonus(block_x, block_y);
	}
}

void GameArkanoid::TrySpawnNewBonus(const uint32_t block_x, const uint32_t block_y)
{
	if(rand_.Next() % g_bonus_drop_inv_chance != 0)
	{
		return;
	}

	// Calculate base probability for each bonus.
	uint32_t bonuses_probability[size_t(BonusType::NumBonuses)];
	for(size_t i = 0; i < size_t(BonusType::NumBonuses); ++i)
	{
		bonuses_probability[i] = 256;
	}

	// Reduce probability for cool bonuses.
	bonuses_probability[size_t(BonusType::ExtraLife)] /= 2;
	bonuses_probability[size_t(BonusType::NextLevel)] /= 4;

	// Do not drop ball split bonus in case if there are already multiple balls.
	if(balls_.size() >= 4)
	{
		bonuses_probability[size_t(BonusType::BallSplit)] = 0;
	}

	// Do not drop two bonuses of same type one after another.
	bonuses_probability[size_t(prev_bonus_type_)] = 0;

	uint32_t total_probability = 0;
	uint32_t probability_integrated[size_t(BonusType::NumBonuses)];
	for(size_t i = 0; i < size_t(BonusType::NumBonuses); ++i)
	{
		probability_integrated[i] = total_probability + bonuses_probability[i];
		total_probability += bonuses_probability[i];
	}
	assert(total_probability > 0);

	// Choose bonus type based on random value and probability.
	const uint32_t rand_value = rand_.Next() % total_probability;
	BonusType bonus_type = BonusType::SlowDown;
	for(size_t i = 0; i < size_t(BonusType::NumBonuses); ++i)
	{
		if(bonuses_probability[i] != 0 && rand_value < probability_integrated[i])
		{
			bonus_type = BonusType(i);
			break;
		}
	}

	Bonus bonus;
	bonus.type = bonus_type;
	bonus.position[0] = IntToFixed16(int32_t(block_x * c_block_width  + c_block_width  / 2));
	bonus.position[1] = IntToFixed16(int32_t(block_y * c_block_height + c_block_height / 2));

	bonuses_.push_back(bonus);

	prev_bonus_type_ = bonus_type;
}

void GameArkanoid::SplitBalls()
{
	const fixed16_t cos_split_angle = 56756;
	const fixed16_t sin_split_angle = 32768;

	for(size_t i = 0, num_balls = balls_.size(); i < num_balls; ++i)
	{
		Ball& ball = balls_[i];
		if(ball.is_attached_to_ship)
		{
			continue;
		}

		const fixed16vec2_t velocity = ball.velocity;

		const fixed16vec2_t velocity0 =
		{
			Fixed16Mul(velocity[0], cos_split_angle) - Fixed16Mul(velocity[1], sin_split_angle),
			Fixed16Mul(velocity[0], sin_split_angle) + Fixed16Mul(velocity[1], cos_split_angle),
		};

		const fixed16vec2_t velocity1 =
		{
			Fixed16Mul(velocity[0], cos_split_angle) - Fixed16Mul(velocity[1], -sin_split_angle),
			Fixed16Mul(velocity[0], -sin_split_angle) + Fixed16Mul(velocity[1], cos_split_angle),
		};

		Ball ball0 = ball;
		ball0.velocity = velocity0;

		Ball ball1 = ball;
		ball1.velocity = velocity1;

		balls_.push_back(ball0);
		balls_.push_back(ball1);
	}
}

void GameArkanoid:: ReleaseStickyBalls()
{
	if(ship_ != std::nullopt && ship_->state == ShipState::Sticky)
	{
		// Fire balls attached to the ship.
		for(Ball& ball : balls_)
		{
			if(ball.is_attached_to_ship)
			{
				ball.is_attached_to_ship = false;
				ball.position[0] += ship_->position[0];
				ball.position[1] += ship_->position[1];
			}
		}
	}
}

void GameArkanoid::CorrectShipPosition()
{
	if(ship_ == std::nullopt)
	{
		return;
	}

	const fixed16_t half_width = IntToFixed16(int32_t(GetShipHalfWidthForState(ship_->state)));
	if(ship_->position[0] - half_width < 0)
	{
		ship_->position[0] = half_width;
	}
	fixed16_t right_border = IntToFixed16(c_field_width * c_block_width);
	if(next_level_exit_is_open_)
	{
		right_border += IntToFixed16(c_ship_half_width_large + 1);
	}
	if(ship_->position[0] + half_width >= right_border)
	{
		ship_->position[0] = right_border - half_width;
	}
}

uint32_t GameArkanoid::GetShipHalfWidthForState(const ShipState ship_state)
{
	switch(ship_state)
	{
	case ShipState::Normal:
	case ShipState::Sticky:
	case ShipState::Turret:
		return c_ship_half_width_normal;
	case ShipState::Large:
		return c_ship_half_width_large;
	};

	assert(false);
	return c_ship_half_width_normal;
}

GameArkanoid::BlockType GameArkanoid::GetBlockTypeForLevelDataByte(const char level_data_byte)
{
	if(
		level_data_byte >= 'A' &&
		uint32_t(level_data_byte) < 'A' + (1 + uint32_t(BlockType::Color15) - uint32_t(BlockType::Color1)))
	{
		return BlockType(uint32_t(BlockType::Color1) + uint32_t(level_data_byte) - 'A');
	}
	if(level_data_byte == '#')
	{
		return BlockType::Concrete;
	}
	if(level_data_byte == '@')
	{
		return BlockType::Color14_15;
	}

	return BlockType::Empty;
}
