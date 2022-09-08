#include "GameArkanoid.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "Sprites.hpp"
#include <cassert>
#include <cmath>

namespace
{

fixed16_t GetBallSpeed()
{
	return g_fixed16_one;
}

} // namespace

GameArkanoid::GameArkanoid()
{
	for(uint32_t y = 4; y < 10; ++y)
	{
		for(uint32_t x = 0; x < c_field_width; ++x)
		field_[x + y * c_field_width].type =
			BlockType((uint32_t(BlockType::Color1) + x) % uint32_t(BlockType::NumTypes));
	}

	Ball ball;
	ball.position =
	{
		IntToFixed16(c_field_width  * c_block_width  / 2),
		IntToFixed16(c_field_height * c_block_height - c_ship_half_height - c_ball_half_size),
	};
	ball.velocity = { 0, -GetBallSpeed() };
	balls_.push_back(ball);

	Ship ship;
	ship.position =
	{
		IntToFixed16(c_field_width * c_block_width / 2),
		IntToFixed16(c_field_height * c_block_height + c_ship_half_height),
	};

	ship_ = ship;
}

void GameArkanoid::Tick(
	const std::vector<SDL_Event>& events,
	const std::vector<bool>& keyboard_state)
{
	(void)keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>();
		}
		if(event.type == SDL_MOUSEMOTION)
		{
			if (ship_ != std::nullopt)
			{
				const fixed16_t sensetivity =g_fixed16_one / 3; // TODO - make this configurable.
				ship_->position[0] += event.motion.xrel * sensetivity;

				const fixed16_t half_width = IntToFixed16(c_ship_half_width_normal);
				if(ship_->position[0] - half_width < 0)
				{
					ship_->position[0] = half_width;
				}
				const fixed16_t right_border = IntToFixed16(c_field_width * c_block_width);
				if(ship_->position[0] + half_width >= right_border)
				{
					ship_->position[0] = right_border - half_width;
				}
			}
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
}

void GameArkanoid::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_offset_x = 15;
	const uint32_t field_offset_y = 15;

	const SpriteBMP block_sprites[]
	{
		Sprites::arkanoid_block_1,
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
			DrawSpriteWithAlphaUnchecked(
				frame_buffer,
				block_sprites[uint32_t(block.type)],
				0,
				field_offset_x + x * c_block_width,
				field_offset_y + y * c_block_height);
		}
	}

	if(ship_ != std::nullopt)
	{
		DrawSpriteWithAlphaUnchecked(
			frame_buffer,
			Sprites::arkanoid_ship,
			0,
			field_offset_x + Fixed16FloorToInt(ship_->position[0]) - c_ship_half_width_normal,
			field_offset_y + Fixed16FloorToInt(ship_->position[1]) - c_ship_half_height);
	}

	for(const Ball& ball : balls_)
	{
		DrawSpriteWithAlphaUnchecked(
			frame_buffer,
			Sprites::arkanoid_ball,
			0,
			field_offset_x + Fixed16FloorToInt(ball.position[0]) - c_ball_half_size,
			field_offset_y + Fixed16FloorToInt(ball.position[1]) - c_ball_half_size);
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
		DrawSpriteWithAlphaUnchecked(
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
		Sprites::arkanoid_trim_segment_side_1,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_1,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
		Sprites::arkanoid_trim_segment_side_0,
	};

	uint32_t trim_side_y = field_offset_y;
	for(const SpriteBMP& sprite : sprites_trim_left)
	{
		DrawSpriteWithAlphaUnchecked(
			frame_buffer,
			sprite,
			0,
			field_offset_x - 10,
			trim_side_y);

		DrawSpriteWithAlphaUnchecked(
			frame_buffer,
			sprite,
			0,
			field_offset_x - 10 + c_block_width * c_field_width + 10,
			trim_side_y);

		trim_side_y += sprite.GetHeight();
	}
}

GameInterfacePtr GameArkanoid::AskForNextGameTransition()
{
	return std::move(next_game_);
}

bool GameArkanoid::UpdateBall(Ball& ball)
{
	ball.position[0] += ball.velocity[0];
	ball.position[1] += ball.velocity[1];

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
			IntToFixed16(x * c_block_width ) - ball_half_size,
			IntToFixed16(y * c_block_height) - ball_half_size,
		};
		const fixed16vec2_t borders_max =
		{
			IntToFixed16((x + 1) * c_block_width ) + ball_half_size,
			IntToFixed16((y + 1) * c_block_height) + ball_half_size,
		};

		if( ball.position[0] <= borders_min[0] || ball.position[0] >= borders_max[0] ||
			ball.position[1] <= borders_min[1] || ball.position[1] >= borders_max[1])
		{
			continue;
		}

		// Hit this block.
		block.type = BlockType::Empty;
		// TODO - count score here.

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

		ball.position[0] = 2 * closest_position[0] - ball.position[0];
		ball.position[1] = 2 * closest_position[1] - ball.position[1];
		if(bounce_vec[0] != 0)
		{
			ball.velocity[0] = -ball.velocity[0];
		}
		if(bounce_vec[1] != 0)
		{
			ball.velocity[1] = -ball.velocity[1];
		}
	} // for blocks.

	if(ship_ != std::nullopt)
	{
		const fixed16_t half_width_extended = IntToFixed16(c_ship_half_width_normal) + ball_half_size;
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

			const fixed16_t speed = GetBallSpeed();
			ball.velocity[0] = Fixed16Mul(speed, angle_cos);
			ball.velocity[1] = -Fixed16Mul(speed, angle_sin);
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
