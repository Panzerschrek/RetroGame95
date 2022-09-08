#include "GameArkanoid.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "Sprites.hpp"
#include <cassert>

GameArkanoid::GameArkanoid()
{
	for(uint32_t i = 0; i < 8; ++i)
	{
		Ball ball;
		ball.position =
		{
			IntToFixed16((i + c_field_width) * c_block_width / 2),
			IntToFixed16((i * 2 + c_field_height) * c_block_height / 2),
		};
		ball.velocity = { 3 * g_fixed16_one / (3 + int32_t(i)), -3 * g_fixed16_one / ( 2 + int32_t(i) ) };
		balls_.push_back(ball);
	}

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

				const fixed16_t half_with = IntToFixed16(c_ship_half_width_normal);
				if(ship_->position[0] - half_with < 0)
				{
					ship_->position[0] = half_with;
				}
				const fixed16_t right_border = IntToFixed16(c_field_width * c_block_width);
				if(ship_->position[0] + half_with >= right_border)
				{
					ship_->position[0] = right_border - half_with;
				}
			}
		}
	}

	for(size_t b = 0; b < balls_.size();)
	{
		Ball& ball = balls_[b];

		ball.position[0] += ball.velocity[0];
		ball.position[1] += ball.velocity[1];

		// Bounce ball from walls.
		const fixed16vec2_t field_size =
		{
			IntToFixed16(c_field_width * c_block_width),
			IntToFixed16((c_field_height + 10) * c_block_height), // Increase lower border to disable floor bounce.
		};
		const fixed16_t ball_half_size = IntToFixed16(c_ball_half_size);
		for(size_t i = 0; i < 2; ++i)
		{
			if(ball.position[i] - ball_half_size < 0)
			{
				ball.velocity[i] = -ball.velocity[i];
				ball.position[i] = ball_half_size * 2 - ball.position[i];
				assert(ball.position[i] >= 0);
			}
			if(ball.position[i] + ball_half_size > field_size[i])
			{
				ball.velocity[i] = -ball.velocity[i];
				ball.position[i] = 2 * field_size[i] - ball_half_size * 2 - ball.position[i];
				assert(ball.position[i] <= field_size[i]);
			}
		}

		if(ball.position[1] + ball_half_size > IntToFixed16((c_field_height) * c_block_height))
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

	const SpriteBMP sprites[]
	{
		Sprites::arkanoid_block_concrete,
		Sprites::arkanoid_block_14_15,
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
	};

	for(uint32_t y = 0; y < c_field_height; ++y)
	{
		for(uint32_t x = 0; x < c_field_width; ++x)
		{
			DrawSpriteWithAlphaUnchecked(
				frame_buffer,
				sprites[ (x + y) % std::size(sprites) ],
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
