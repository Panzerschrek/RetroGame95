#include "GameArkanoid.hpp"
#include "ArkanoidLevels.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "GamesDrawCommon.hpp"
#include "GameTetris.hpp"
#include "Progress.hpp"
#include "Sprites.hpp"
#include "Strings.hpp"
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
	OpenGame(GameId::Arkanoid);

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

	const uint32_t field_offset_x = g_arkanoid_field_offset_x;
	const uint32_t field_offset_y = g_arkanoid_field_offset_y;

	DrawArkanoidField(frame_buffer, field_);

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

	DrawArkanoidFieldBorder(frame_buffer, next_level_exit_is_open_);

	DrawArakoindStats(frame_buffer, level_, score_);

	if(tick_ < level_start_animation_end_tick_)
	{
		DrawArkanoidLevelStartSplash(frame_buffer, level_);
	}

	if(game_over_)
	{
		DrawTextCentered(
			frame_buffer,
			g_cga_palette[9],
			field_offset_x + c_block_width  * c_field_width  / 2,
			field_offset_y + c_block_height * (c_field_height - 5),
			Strings::arkanoid_game_over);
	}
}

GameInterfacePtr GameArkanoid::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameArkanoid::ProcessLogic(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	if(keyboard_state.size() > SDL_SCANCODE_LEFT  && keyboard_state[SDL_SCANCODE_LEFT ])
	{
		ship_->position[0] -= g_arkanoid_ship_keyboard_move_sensetivity;
		CorrectShipPosition();
	}
	if(keyboard_state.size() > SDL_SCANCODE_RIGHT && keyboard_state[SDL_SCANCODE_RIGHT])
	{
		ship_->position[0] += g_arkanoid_ship_keyboard_move_sensetivity;
		CorrectShipPosition();
	}

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_MOUSEMOTION)
		{
			if (ship_ != std::nullopt)
			{
				ship_->position[0] += event.motion.xrel * g_arkanoid_ship_mouse_move_sensetivity;
				CorrectShipPosition();
			}
		}
		if(event.type == SDL_KEYDOWN &&
			(event.key.keysym.scancode == SDL_SCANCODE_RCTRL ||
			 event.key.keysym.scancode == SDL_SCANCODE_LCTRL ||
			 event.key.keysym.scancode == SDL_SCANCODE_SPACE))
		{
			ProcessShootRequest();
		}
		if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 1)
		{
			ProcessShootRequest();
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

		sound_player_.PlaySound(SoundId::CharacterDeath);
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
			const ArkanoidBlock& block = field_[x + y * c_field_width];
			if(block.type != ArkanoidBlockType::Empty)
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

void GameArkanoid::ProcessShootRequest()
{
	if(ship_ == std::nullopt)
	{
		return;
	}

	ReleaseStickyBalls();
	if(ship_->state == ShipState::Sticky)
	{
		// Reset sticky state after first shot.
		ship_->state = ShipState::Normal;
	}

	if(ship_->state == ShipState::Turret)
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

	FillArkanoidField(field_, arkanoid_levels[(level_ - 1) % std::size(arkanoid_levels)]);

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
		ArkanoidBlock& block = field_[x + y * c_field_width];
		if(block.type == ArkanoidBlockType::Empty)
		{
			continue;
		}

		if(MakeCollisionBetweenObjectAndBox(
			{
				IntToFixed16(int32_t(x * c_block_width) ),
				IntToFixed16(int32_t(y * c_block_height)),
			},
			{
				IntToFixed16(int32_t((x + 1) * c_block_width )),
				IntToFixed16(int32_t((y + 1) * c_block_height)),
			},
			{ball_half_size, ball_half_size},
			ball.position,
			ball.velocity))
		{
			// Hit this block.
			DamageBlock(x, y);
			sound_player_.PlaySound(SoundId::ArkanoidBallHit);
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
		ArkanoidBlock& block = field_[x + y * c_field_width];
		if(block.type == ArkanoidBlockType::Empty)
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
	ArkanoidBlock& block = field_[ block_x + block_y * c_field_width ];
	if(block.type == ArkanoidBlockType::Empty)
	{
		return;
	}

	// TODO - tune score calculation.
	score_ += 16;

	assert(block.health > 0);
	--block.health;

	if(block.health == 0)
	{
		block.type = ArkanoidBlockType::Empty;
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
