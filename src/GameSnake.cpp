#include "GameSnake.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "SpriteBMP.hpp"
#include "Sprites.hpp"

GameSnake::GameSnake(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
	SpawnSnake();
}

void GameSnake::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void) keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
		if(event.type == SDL_KEYDOWN && snake_ != std::nullopt)
		{
			// Turn snake, but do not allow to turn directly towards the neck.
			if(event.key.keysym.scancode == SDL_SCANCODE_LEFT &&
				snake_->segments[0].position[0] <= snake_->segments[1].position[0])
			{
				snake_->direction = SnakeDirection::XMinus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT &&
				snake_->segments[0].position[0] >= snake_->segments[1].position[0])
			{
				snake_->direction = SnakeDirection::XPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_DOWN &&
				snake_->segments[0].position[1] >= snake_->segments[1].position[1])
			{
				snake_->direction = SnakeDirection::YPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_UP &&
				snake_->segments[0].position[1] <= snake_->segments[1].position[1])
			{
				snake_->direction = SnakeDirection::YMinus;
			}
		}
	}

	++num_ticks_;

	if(num_ticks_ % 60 == 0)
	{
		MoveSnake();
	}
}

void GameSnake::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_offset_x = 10;
	const uint32_t field_offset_y = 10;

	if(snake_ != std::nullopt)
	{
		const SpriteBMP head_sprite(Sprites::snake_head);
		const SpriteBMP body_segment_sprite(Sprites::snake_body_segment);
		const SpriteBMP body_segment_angle_sprite(Sprites::snake_body_segment_angle);

		const SpriteBMP tail_sprite(Sprites::snake_tail);

		for(size_t i = 0; i < snake_->segments.size(); ++i)
		{
			const SnakeSegment& segment = snake_->segments[i];

			SpriteBMP sprite = body_segment_sprite;
			auto draw_fn = DrawSpriteWithAlphaUnchecked;
			int32_t sprite_offset_x = 0;
			int32_t sprite_offset_y = 0;

			if(i == 0)
			{
				sprite = head_sprite;

				const SnakeSegment& next_segment = snake_->segments[i + 1];

				if(segment.position[0] > next_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaUncheckedRotate270;
				}
				if(segment.position[0] < next_segment.position[0])
				{
					sprite_offset_x = -5;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate90;
				}
				if(segment.position[1] > next_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlphaUnchecked;
				}
				if(segment.position[1] < next_segment.position[1])
				{
					sprite_offset_y = -5;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate180;
				}
			}
			else if(i == snake_->segments.size() - 1)
			{
				sprite = tail_sprite;

				const SnakeSegment& prev_segment = snake_->segments[i - 1];
				if(segment.position[0] >prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaUncheckedRotate90;
				}
				if(segment.position[0] < prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaUncheckedRotate270;
				}
				if(segment.position[1] > prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlphaUncheckedRotate180;
				}
				if(segment.position[1] < prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlphaUnchecked;
				}
			}
			else
			{
				const SnakeSegment& next_segment = snake_->segments[i + 1];
				const SnakeSegment& prev_segment = snake_->segments[i - 1];
				if(next_segment.position[0] < segment.position[0] && segment.position[0] < prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaUncheckedRotate90;
				}
				else if(next_segment.position[0] > segment.position[0] && segment.position[0] > prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaUncheckedRotate270;
				}
				else if(next_segment.position[1] < segment.position[1] && segment.position[1] < prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlphaUncheckedRotate180;
				}
				else if(next_segment.position[1] > segment.position[1] && segment.position[1] > prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlphaUnchecked;
				}
				else if(next_segment.position[0] > segment.position[0] && prev_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate180;
				}
				else if(prev_segment.position[0] > segment.position[0] && next_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate180;
				}
				else if(next_segment.position[0] > segment.position[0] && prev_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate90;
				}
				else if(prev_segment.position[0] > segment.position[0] && next_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate90;
				}
				else if(next_segment.position[0] < segment.position[0] && prev_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate270;
				}
				else if(next_segment.position[0] < segment.position[0] && prev_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUnchecked;
				}
				else if(prev_segment.position[0] < segment.position[0] && next_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUncheckedRotate270;
				}
				else if(prev_segment.position[0] < segment.position[0] && next_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaUnchecked;
				}
			}

			draw_fn(
				frame_buffer,
				sprite,
				0,
				uint32_t(int32_t(field_offset_x + segment.position[0] * c_block_size) + sprite_offset_x),
				uint32_t(int32_t(field_offset_y + segment.position[1] * c_block_size) + sprite_offset_y));
		}
	}
}

GameInterfacePtr GameSnake::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameSnake::SpawnSnake()
{
	Snake snake;
	snake.direction = SnakeDirection::YPlus;

	for(uint32_t i = 0; i < 5; ++i)
	{
		SnakeSegment segment;
		segment.position[0] = c_field_width / 2;
		segment.position[1] = c_field_height / 2 - i;
		snake.segments.push_back(segment);
	}

	snake_ = std::move(snake);
}

void GameSnake::MoveSnake()
{
	if(snake_ == std::nullopt)
	{
		return;
	}

	SnakeSegment new_segment = snake_->segments.front();
	// TODO - check for walls collision here.
	switch(snake_->direction)
	{
	case SnakeDirection::XPlus:
		new_segment.position[0] += 1;
		break;
	case SnakeDirection::XMinus:
		new_segment.position[0] -= 1;
		break;
	case SnakeDirection::YPlus:
		new_segment.position[1] += 1;
		break;
	case SnakeDirection::YMinus:
		new_segment.position[1] -= 1;
		break;
	}

	snake_->segments.insert(snake_->segments.begin(), new_segment);
	snake_->segments.pop_back();

	// TODO - check for obstacles collision, self-intersection, food consumption.
}
