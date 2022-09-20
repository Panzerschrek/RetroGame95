#include "GameSnake.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "GamePacman.hpp"
#include "Progress.hpp"
#include "SpriteBMP.hpp"
#include "Sprites.hpp"
#include <cassert>

namespace
{

const uint32_t g_max_lifes = 6;
const uint32_t g_max_level = 3;

const uint32_t g_grow_points_per_food_piece_small = 3;
const uint32_t g_grow_points_per_food_piece_medium = 5;
const uint32_t g_grow_points_per_food_piece_large = 7;

const uint32_t g_extra_life_spawn_inv_chance = 20;

const uint32_t g_field_start_animation_duration = 240;
const uint32_t g_death_animation_duration = 180;
const uint32_t g_death_animation_flicker_duration = 12;

uint32_t GetLengthForNextLevelTransition(const uint32_t level)
{
	return 100 + 10 * level;
}

uint32_t GetSpeedForLevel(const uint32_t level)
{
	return 240 / (3 + level);
}

} // namespace

GameSnake::GameSnake(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
	OpenGame(GameId::Snake);

	NextLevel();
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
		if(event.type == SDL_KEYDOWN &&
			snake_ != std::nullopt && death_animation_end_tick_ == std::nullopt && field_start_animation_end_tick_ == std::nullopt)
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

	++tick_;

	if(game_over_)
	{
		return;
	}

	TrySpawnTetrisPiece();

	const uint32_t speed = GetSpeedForLevel(level_);
	if(tick_ % speed == 0)
	{
		MoveSnake();
	}
	if(tick_ % speed == speed / 2)
	{
		MoveTetrisPieceDown();
	}

	if(field_start_animation_end_tick_ != std::nullopt && tick_ > *field_start_animation_end_tick_)
	{
		field_start_animation_end_tick_ = std::nullopt;
	}

	if(death_animation_end_tick_ != std::nullopt && tick_ >= *death_animation_end_tick_)
	{
		death_animation_end_tick_ = std::nullopt;
		if(lifes_ > 0)
		{
			--lifes_;
			NewField();
		}
		else
		{
			game_over_ = true;
			snake_ = std::nullopt;
		}
	}

	if(!game_over_ && death_animation_end_tick_ == std::nullopt &&
		snake_ != std::nullopt && snake_->segments.size() >= GetLengthForNextLevelTransition(level_))
	{
		if(level_ >= g_max_level)
		{
			next_game_ = std::make_unique<GamePacman>(sound_player_);
		}
		else
		{
			NextLevel();
		}
	}
}

void GameSnake::Draw(const FrameBuffer frame_buffer) const
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_offset_x = 10;
	const uint32_t field_offset_y = 10;

	DrawHorisontalLine(
		frame_buffer,
		g_color_white,
		field_offset_x - 1,
		field_offset_y - 1,
		c_block_size * c_field_width + 2);
	DrawHorisontalLine(
		frame_buffer,
		g_color_white,
		field_offset_x - 1,
		field_offset_y + c_block_size * c_field_height,
		c_block_size * c_field_width + 2);

	DrawVerticaLine(
		frame_buffer,
		g_color_white,
		field_offset_x - 1,
		field_offset_y - 1,
		c_block_size * c_field_height + 2);
	DrawVerticaLine(
		frame_buffer,
		g_color_white,
		field_offset_x + c_block_size * c_field_width,
		field_offset_y - 1,
		c_block_size * c_field_height + 2);

	const SpriteBMP bonus_sprites[]
	{
		Sprites::snake_food_small,
		Sprites::snake_food_medium,
		Sprites::snake_food_large,
		Sprites::snake_extra_life,
	};

	for(const Bonus& bonus : bonuses_)
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			bonus_sprites[size_t(bonus.type)],
			0,
			field_offset_x + bonus.position[0] * c_block_size,
			field_offset_y + bonus.position[1] * c_block_size);
	}

	if(snake_ != std::nullopt &&
		field_start_animation_end_tick_ == std::nullopt &&
		(death_animation_end_tick_ == std::nullopt || (tick_ / g_death_animation_flicker_duration) % 2 == 0))
	{
		const SpriteBMP head_sprite(Sprites::snake_head);
		const SpriteBMP body_segment_sprite(Sprites::snake_body_segment);
		const SpriteBMP body_segment_angle_sprite(Sprites::snake_body_segment_angle);

		const SpriteBMP tail_sprite(Sprites::snake_tail);

		for(size_t i = snake_->segments.size() - 1; ;)
		{
			const SnakeSegment& segment = snake_->segments[i];

			SpriteBMP sprite = body_segment_sprite;
			auto draw_fn = DrawSpriteWithAlpha;
			int32_t sprite_offset_x = 0;
			int32_t sprite_offset_y = 0;

			if(i == 0)
			{
				sprite = head_sprite;

				const SnakeSegment& next_segment = snake_->segments[i + 1];

				if(segment.position[0] > next_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaRotate270;
				}
				if(segment.position[0] < next_segment.position[0])
				{
					sprite_offset_x = -5;
					draw_fn = DrawSpriteWithAlphaRotate90;
				}
				if(segment.position[1] > next_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlpha;
				}
				if(segment.position[1] < next_segment.position[1])
				{
					sprite_offset_y = -5;
					draw_fn = DrawSpriteWithAlphaRotate180;
				}
			}
			else if(i == snake_->segments.size() - 1)
			{
				sprite = tail_sprite;

				const SnakeSegment& prev_segment = snake_->segments[i - 1];
				if(segment.position[0] >prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaRotate90;
				}
				if(segment.position[0] < prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaRotate270;
				}
				if(segment.position[1] > prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlphaRotate180;
				}
				if(segment.position[1] < prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlpha;
				}
			}
			else
			{
				const SnakeSegment& next_segment = snake_->segments[i + 1];
				const SnakeSegment& prev_segment = snake_->segments[i - 1];
				if(next_segment.position[0] < segment.position[0] && segment.position[0] < prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaRotate90;
				}
				else if(next_segment.position[0] > segment.position[0] && segment.position[0] > prev_segment.position[0])
				{
					draw_fn = DrawSpriteWithAlphaRotate270;
				}
				else if(next_segment.position[1] < segment.position[1] && segment.position[1] < prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlphaRotate180;
				}
				else if(next_segment.position[1] > segment.position[1] && segment.position[1] > prev_segment.position[1])
				{
					draw_fn = DrawSpriteWithAlpha;
				}
				else if(next_segment.position[0] > segment.position[0] && prev_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaRotate180;
				}
				else if(prev_segment.position[0] > segment.position[0] && next_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaRotate180;
				}
				else if(next_segment.position[0] > segment.position[0] && prev_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaRotate90;
				}
				else if(prev_segment.position[0] > segment.position[0] && next_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = 1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaRotate90;
				}
				else if(next_segment.position[0] < segment.position[0] && prev_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaRotate270;
				}
				else if(next_segment.position[0] < segment.position[0] && prev_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlpha;
				}
				else if(prev_segment.position[0] < segment.position[0] && next_segment.position[1] > segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = 1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlphaRotate270;
				}
				else if(prev_segment.position[0] < segment.position[0] && next_segment.position[1] < segment.position[1])
				{
					sprite_offset_x = -1;
					sprite_offset_y = -1;
					sprite = body_segment_angle_sprite;
					draw_fn = DrawSpriteWithAlpha;
				}
			}

			draw_fn(
				frame_buffer,
				sprite,
				0,
				uint32_t(int32_t(field_offset_x + segment.position[0] * c_block_size) + sprite_offset_x),
				uint32_t(int32_t(field_offset_y + segment.position[1] * c_block_size) + sprite_offset_y));

			if(i == 0)
			{
				break;
			}
			--i;
		} // for snake segments
	}

	if(tetris_active_piece_ != std::nullopt)
	{
		const SpriteBMP sprites[g_tetris_num_piece_types]
		{
			Sprites::tetris_block_4,
			Sprites::tetris_block_7,
			Sprites::tetris_block_5,
			Sprites::tetris_block_1,
			Sprites::tetris_block_2,
			Sprites::tetris_block_6,
			Sprites::tetris_block_3,
		};

		for(const auto& piece_block : tetris_active_piece_->blocks)
		{
			if(piece_block[0] >= 0 && piece_block[0] < int32_t(c_field_width) &&
				piece_block[1] >= 0 && piece_block[1] < int32_t(c_field_height))
			{
				DrawSpriteWithAlpha(
					frame_buffer,
					sprites[uint32_t(tetris_active_piece_->type) - 1],
					0,
					field_offset_x + uint32_t(piece_block[0]) * c_block_size,
					field_offset_y + uint32_t(piece_block[1]) * c_block_size);
			}
		}
	}

	char text[64];

	if(field_start_animation_end_tick_ != std::nullopt)
	{
		std::snprintf(text, sizeof(text), "level %d", level_);

		DrawTextCentered(
			frame_buffer,
			g_color_white,
			field_offset_x + c_block_size  * c_field_width  / 2,
			field_offset_y + c_block_size * c_field_height / 2,
			text);
	}
	if(game_over_)
	{
		DrawTextCentered(
			frame_buffer,
			g_color_white,
			field_offset_x + c_block_size  * c_field_width  / 2,
			field_offset_y + c_block_size * c_field_height / 2,
			"game over");
	}

	std::snprintf(
		text,
		sizeof(text),
		"length %3d  lifes %1d  level %1d  score %4d",
		uint32_t(snake_ == std::nullopt ? 0 : snake_->segments.size()),
		lifes_,
		level_,
		score_);
	DrawText(frame_buffer, g_color_white, 0, frame_buffer.height - 10, text);

}

GameInterfacePtr GameSnake::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameSnake::NextLevel()
{
	++level_;

	NewField();
}

void GameSnake::NewField()
{
	SpawnSnake();

	tetris_active_piece_ = std::nullopt;

	// Clear all bonuses.
	for(Bonus& bonus : bonuses_)
	{
		bonus.position = { 9999, 9999 };
	}

	// Spawn new bonses.
	for(Bonus& bonus : bonuses_)
	{
		bonus = SpawnBonus();
	}

	field_start_animation_end_tick_ = tick_ + g_field_start_animation_duration;
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
	if(death_animation_end_tick_ != std::nullopt || field_start_animation_end_tick_ != std::nullopt)
	{
		return;
	}

	SnakeSegment new_segment = snake_->segments.front();
	bool hit_obstacle = false;
	switch(snake_->direction)
	{
	case SnakeDirection::XPlus:
		if(new_segment.position[0] < c_field_width - 1)
		{
			new_segment.position[0] += 1;
		}
		else
		{
			hit_obstacle = true;
		}
		break;
	case SnakeDirection::XMinus:
		if(new_segment.position[0] > 0)
		{
			new_segment.position[0] -= 1;
		}
		else
		{
			hit_obstacle = true;
		}
		break;
	case SnakeDirection::YPlus:
		if(new_segment.position[1] < c_field_height - 1)
		{
			new_segment.position[1] += 1;
		}
		else
		{
			hit_obstacle = true;
		}
		break;
	case SnakeDirection::YMinus:
		if(new_segment.position[1] > 0)
		{
			new_segment.position[1] -= 1;
		}
		else
		{
			hit_obstacle = true;
		}
		break;
	}

	if(tetris_active_piece_ != std::nullopt)
	{
		for(const TetrisPieceBlock& block : tetris_active_piece_->blocks)
		{
			hit_obstacle |=
				block[0] == int32_t(new_segment.position[0]) &&
				block[1] == int32_t(new_segment.position[1]);
		}
	}

	// TODO - check for obstacles collision.

	if(hit_obstacle)
	{
		OnSnakeDeath();
		return;
	}

	sound_player_.PlaySound(SoundId::TetrisFigureStep);

	// Check for bonuses collision.
	for(Bonus& bonus : bonuses_)
	{
		if(new_segment.position == bonus.position)
		{
			// Pick-up the bonus.
			sound_player_.PlaySound(SoundId::SnakeBonusEat);

			switch(bonus.type)
			{
			case BonusType::FoodSmall:
				snake_->grow_points_ += g_grow_points_per_food_piece_small;
				break;
			case BonusType::FoodMedium:
				snake_->grow_points_ += g_grow_points_per_food_piece_medium;
				break;
			case BonusType::FoodLarge:
				snake_->grow_points_ += g_grow_points_per_food_piece_large;
				break;
			case BonusType::ExtraLife:
				lifes_ = std::min(lifes_ + 1, g_max_lifes);
				break;
			case BonusType::NumTypes:
				assert(false);
				break;
			};
			score_ += 10; // TODO - make score dependent on level.

			// Respawn bonus.
			bonus = SpawnBonus();
		}
	}

	snake_->segments.insert(snake_->segments.begin(), new_segment);
	if(snake_->grow_points_ > 0)
	{
		--snake_->grow_points_;
	}
	else
	{
		snake_->segments.pop_back();
	}

	for(size_t i = 1; i < snake_->segments.size(); ++i)
	{
		if(snake_->segments.front().position == snake_->segments[i].position)
		{
			OnSnakeDeath();
			return;
		}
	}
}

void GameSnake::OnSnakeDeath()
{
	death_animation_end_tick_ = tick_ + g_death_animation_duration;
	sound_player_.PlaySound(SoundId::SnakeDeath);
}

void GameSnake::MoveTetrisPieceDown()
{
	if(tetris_active_piece_ == std::nullopt)
	{
		return;
	}

	if(death_animation_end_tick_ != std::nullopt)
	{
		// Stop tetris piece on snake death.
		return;
	}

	if(snake_ != std::nullopt)
	{
		bool hit_snake = false;
		for(const TetrisPieceBlock& block : tetris_active_piece_->blocks)
		{
			for(const SnakeSegment& segment : snake_->segments)
			{
				hit_snake |=
					int32_t(segment.position[0]) == block[0] &&
					int32_t(segment.position[1]) == block[1] + 1;
			}
		}

		if(hit_snake)
		{
			OnSnakeDeath();
			return;
		}
	}

	// Can move.
	// TODO - place piece into game field (in needed).
	for(TetrisPieceBlock& block : tetris_active_piece_->blocks)
	{
		block[1] += 1;
	}
}

bool GameSnake::IsPositionFree(const std::array<uint32_t, 2>& position) const
{
	if(snake_ != std::nullopt)
	{
		for(const SnakeSegment& segment : snake_->segments)
		{
			if(position == segment.position)
			{
				return false;
			}
		}
	}

	for(const Bonus& bonus : bonuses_)
	{
		if(bonus.position == position)
		{
			return false;
		}
	}

	return true;
}

std::array<uint32_t, 2> GameSnake::GetRandomPosition()
{
	return {rand_.Next() % c_field_width, rand_.Next() % c_field_height };
}

std::array<uint32_t, 2> GameSnake::GetRandomFreePosition()
{
	while(true)
	{
		const std::array<uint32_t, 2> position = GetRandomPosition();
		if(IsPositionFree(position))
		{
			return position;
		}
	}
}

GameSnake::Bonus GameSnake::SpawnBonus()
{
	Bonus bonus;

	bonus.position = GetRandomFreePosition();

	if(rand_.Next() % g_extra_life_spawn_inv_chance == 0)
	{
		bonus.type = BonusType::ExtraLife;
	}
	else
	{
		bonus.type = BonusType(rand_.Next() % uint32_t(BonusType::ExtraLife));
	}

	return bonus;
}

void GameSnake::TrySpawnTetrisPiece()
{
	if(tetris_active_piece_ != std::nullopt)
	{
		return;
	}
	if(death_animation_end_tick_ != std::nullopt || field_start_animation_end_tick_ != std::nullopt)
	{
		return;
	}

	if(rand_.Next() % (20 * GameInterface::c_update_frequency / 10) != 17)
	{
		return;
	}

	const TetrisBlock type = TetrisBlock(uint32_t(TetrisBlock::I) + rand_.Next() % g_tetris_num_piece_types);
	const TetrisPieceBlocks blocks = g_tetris_pieces_blocks[uint32_t(type) - uint32_t(TetrisBlock::I)];

	int32_t min_x = 99999, max_x = -9999;
	for(const TetrisPieceBlock& block : blocks)
	{
		min_x = std::min(min_x, block[0]);
		max_x = std::max(max_x, block[0]);
	}

	// Perform spawn possibility check for multiple random positions across game field.
	// Spawn piece only over areas where there is no snake body.
	const int32_t min_dx = -min_x;
	const int32_t max_dx = int32_t(c_field_width) - max_x - 1;
	for(uint32_t i = 0; i < c_field_width * 4; ++i)
	{
		const int32_t dx = min_dx + int32_t(rand_.Next() % uint32_t(max_dx - min_dx));
		const int32_t cur_min_x = min_x + dx;
		const int32_t cur_max_x = max_x + dx;

		if(snake_ != std::nullopt)
		{
			bool may_hit_snake = false;
			for(const SnakeSegment& segment : snake_->segments)
			{
				if(int32_t(segment.position[0]) >= cur_min_x && int32_t(segment.position[0]) <= cur_max_x)
				{
					may_hit_snake = true;
					break;
				}
			}

			if(may_hit_snake)
			{
				continue;
			}
		}

		TetrisPieceBlocks blocks_shifted = blocks;
		for(TetrisPieceBlock& block : blocks_shifted)
		{
			block[0] += dx;
			assert(block[0] >= 0 && block[0] < int32_t(c_field_width));
		}

		TetrisPiece piece;
		piece.type = type;
		piece.blocks = blocks_shifted;
		tetris_active_piece_ = piece;
		return;
	} // Try to spawn a piece.
}
