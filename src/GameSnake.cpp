#include "GameSnake.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "GamePacman.hpp"
#include "GamesDrawCommon.hpp"
#include "Progress.hpp"
#include "SpriteBMP.hpp"
#include "Sprites.hpp"
#include "String.hpp"
#include "Strings.hpp"
#include <cassert>
#include <cstring>

namespace
{

const uint32_t g_max_lifes = 6;
const uint32_t g_max_level = 3;

const uint32_t g_min_snake_len = 3;

const uint32_t g_grow_points_per_food_piece_small = 3;
const uint32_t g_grow_points_per_food_piece_medium = 5;
const uint32_t g_grow_points_per_food_piece_large = 7;

const uint32_t g_extra_life_spawn_inv_chance = 20;

const uint32_t g_field_start_animation_duration = 240;
const uint32_t g_death_animation_duration = 180;
const uint32_t g_death_animation_flicker_duration = 12;

const uint32_t g_transition_time_out_of_borders_elements_disappear = 30 + g_field_start_animation_duration + GameInterface::c_update_frequency;
const uint32_t g_transition_time_field_change = g_transition_time_out_of_borders_elements_disappear + GameInterface::c_update_frequency;
const uint32_t g_transition_time_field_border_tile_change = g_transition_time_field_change + GameInterface::c_update_frequency;
const uint32_t g_transition_time_bonuses_show = g_transition_time_field_border_tile_change + GameInterface::c_update_frequency;
const uint32_t g_transition_time_snake_visual_change = g_transition_time_bonuses_show + GameInterface::c_update_frequency;
const uint32_t g_transition_time_stats_show = g_transition_time_snake_visual_change + GameInterface::c_update_frequency;
const uint32_t g_transition_time_change_end = g_transition_time_snake_visual_change;

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

	if(tick_ < g_transition_time_snake_visual_change &&
		field_start_animation_end_tick_ == std::nullopt && level_end_animation_end_tick_ == std::nullopt)
	{
		ManipulateSnakeAsTetrisPiece(events);
	}

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
		if(event.type == SDL_KEYDOWN &&
			snake_ != std::nullopt &&
			death_animation_end_tick_ == std::nullopt &&
			field_start_animation_end_tick_ == std::nullopt &&
			tick_ >= g_transition_time_snake_visual_change)
		{
			// Turn snake, but do not allow to turn directly towards the neck.
			if(event.key.keysym.scancode == SDL_SCANCODE_LEFT &&
				snake_->segments[0].position[0] <= snake_->segments[1].position[0])
			{
				snake_->direction = GridDirection::XMinus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT &&
				snake_->segments[0].position[0] >= snake_->segments[1].position[0])
			{
				snake_->direction = GridDirection::XPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_DOWN &&
				snake_->segments[0].position[1] >= snake_->segments[1].position[1])
			{
				snake_->direction = GridDirection::YPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_UP &&
				snake_->segments[0].position[1] <= snake_->segments[1].position[1])
			{
				snake_->direction = GridDirection::YMinus;
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
		if(death_animation_end_tick_ == std::nullopt &&
			field_start_animation_end_tick_ == std::nullopt && level_end_animation_end_tick_ == std::nullopt)
		{
			if(tick_ < g_transition_time_snake_visual_change)
			{
				MoveSnakeAsTetrisPiece();
			}
			else
			{
				MoveSnake();
			}
		}
	}
	if(tick_ % speed == speed / 2)
	{
		MoveTetrisPieceDown();
	}

	for(size_t b = 0; b < arkanoid_balls_.size();)
	{
		if(UpdateArkanoidBall(arkanoid_balls_[b]))
		{
			// This ball is dead.
			if(b + 1 < arkanoid_balls_.size())
			{
				arkanoid_balls_[b] = arkanoid_balls_.back();
			}
			arkanoid_balls_.pop_back();
		}
		else
		{
			++b;
		}
	} // for balls.

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
		EndLevel();
	}

	if(level_end_animation_end_tick_ != std::nullopt && tick_ >= *level_end_animation_end_tick_)
	{
		level_end_animation_end_tick_ = std::nullopt;
		NextLevel();
	}
}

void GameSnake::Draw(const FrameBuffer frame_buffer) const
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const uint32_t field_offset_x = c_block_size;
	const uint32_t field_offset_y = c_block_size;

	if(tick_ < g_transition_time_out_of_borders_elements_disappear)
	{
		DrawTetrisNextPiece(frame_buffer, TetrisBlock::I);
		DrawTetrisStats(frame_buffer, level_, score_);
	}

	if(tick_ < g_transition_time_field_change)
	{
		DrawTetrisFieldBorder(frame_buffer, false);
	}
	else
	{
		const SpriteBMP border_sprite(
				tick_ >= g_transition_time_field_border_tile_change
					? Sprites::snake_field_border
					: Sprites::tetris_block_8);

		for(uint32_t x = 0; x < c_field_width; ++x)
		{
			DrawSprite(
				frame_buffer,
				border_sprite,
				field_offset_x + x * c_block_size,
				field_offset_y - c_block_size);
			DrawSprite(
				frame_buffer,
				border_sprite,
				field_offset_x + x * c_block_size,
				field_offset_y + c_field_height * c_block_size);
		}
		for(uint32_t y = 0; y < c_field_height + 2; ++y)
		{
			DrawSprite(
				frame_buffer,
				border_sprite,
				field_offset_x - 1 * c_block_size,
				field_offset_y + y * c_block_size - c_block_size);
			DrawSprite(
				frame_buffer,
				border_sprite,
				field_offset_x + c_field_width * c_block_size,
				field_offset_y + y * c_block_size - c_block_size);
		}
	}

	const SpriteBMP tetris_blocks_sprites[g_tetris_num_piece_types]
	{
		Sprites::tetris_block_4,
		Sprites::tetris_block_7,
		Sprites::tetris_block_5,
		Sprites::tetris_block_1,
		Sprites::tetris_block_2,
		Sprites::tetris_block_6,
		Sprites::tetris_block_3,
	};

	DrawTetrisField(frame_buffer, field_offset_x, field_offset_y, tetris_field_, c_field_width, c_field_height);

	if(tetris_active_piece_ != std::nullopt)
	{
		for(const auto& piece_block : tetris_active_piece_->blocks)
		{
			if(piece_block[0] >= 0 && piece_block[0] < int32_t(c_field_width) &&
				piece_block[1] >= -1 && piece_block[1] < int32_t(c_field_height))
			{
				DrawSpriteWithAlpha(
					frame_buffer,
					tetris_blocks_sprites[uint32_t(tetris_active_piece_->type) - 1],
					0,
					field_offset_x + uint32_t(piece_block[0]) * c_block_size,
					uint32_t(int32_t(field_offset_y) + piece_block[1] * int32_t(c_block_size)));
			}
		}
	}

	if(tick_ >= g_transition_time_bonuses_show)
	{
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
	}

	if(snake_ != std::nullopt && tick_ < g_transition_time_snake_visual_change)
	{
		for(const SnakeSegment& segment : snake_->segments)
		{
			DrawSprite(
				frame_buffer,
				Sprites::tetris_block_4,
				field_offset_x + segment.position[0] * c_block_size,
				field_offset_y + segment.position[1] * c_block_size);
		}
	}

	if(snake_ != std::nullopt &&
		field_start_animation_end_tick_ == std::nullopt &&
		(death_animation_end_tick_ == std::nullopt || (tick_ / g_death_animation_flicker_duration) % 2 == 0) &&
		tick_ >= g_transition_time_snake_visual_change)
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

	for(const ArkanoidBall& arkanoid_ball: arkanoid_balls_)
	{
		const SpriteBMP sprite(Sprites::arkanoid_ball);
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			field_offset_x + uint32_t(Fixed16FloorToInt(int32_t(c_block_size) * arkanoid_ball.position[0])) - sprite.GetWidth () / 2,
			field_offset_y + uint32_t(Fixed16FloorToInt(int32_t(c_block_size) * arkanoid_ball.position[1])) - sprite.GetHeight() / 2);
	}

	char text[64];

	if(field_start_animation_end_tick_ != std::nullopt)
	{
		std::strcpy(text, Strings::snake_level);
		std::strcpy(text + std::strlen(text), " ");
		const size_t len = std::strlen(text);
		NumToString(text + len, sizeof(text) - len, level_, 1);

		DrawTextCentered(
			frame_buffer,
			g_color_white,
			field_offset_x + c_block_size  * c_field_width  / 2,
			field_offset_y + c_block_size * c_field_height / 2,
			text);
	}
	if(level_end_animation_end_tick_ != std::nullopt)
	{
		DrawTextCentered(
			frame_buffer,
			g_color_white,
			field_offset_x + c_block_size  * c_field_width  / 2,
			field_offset_y + c_block_size * c_field_height / 2,
			Strings::snake_level_completed);
	}
	if(game_over_)
	{
		DrawTextCentered(
			frame_buffer,
			g_color_white,
			field_offset_x + c_block_size  * c_field_width  / 2,
			field_offset_y + c_block_size * c_field_height / 2,
			Strings::snake_game_over);
	}

	if(tick_ >= g_transition_time_stats_show)
	{
		DrawSnakeStats(
			frame_buffer,
			uint32_t(snake_ == std::nullopt ? 0 : snake_->segments.size()),
			lifes_,
			level_,
			score_);
	}
}

GameInterfacePtr GameSnake::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GameSnake::EndLevel()
{
	if(level_end_animation_end_tick_ != std::nullopt)
	{
		return;
	}

	const MusicId music_id = MusicId::InMeinemRaum;
	sound_player_.PlayMusic(music_id);

	level_end_animation_end_tick_ =
		tick_ +
		uint32_t(Fixed16RoundToInt(g_fixed16_one + sound_player_.GetMelodyDuration(music_id))) * GameInterface::c_update_frequency;
}

void GameSnake::NextLevel()
{
	if(level_ >= g_max_level)
	{
		next_game_ = std::make_unique<GamePacman>(sound_player_);
		return;
	}

	++level_;

	NewField();
}

void GameSnake::NewField()
{
	SpawnSnake();

	for(TetrisBlock& block : tetris_field_)
	{
		block = TetrisBlock::Empty;
	}
	tetris_active_piece_ = std::nullopt;

	arkanoid_balls_.clear();

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
	snake.direction = GridDirection::YPlus;

	const uint32_t c_initial_length = 4;
	const uint32_t offset = level_ <= 1 ? (c_initial_length - 1) : ((c_field_height + c_initial_length) / 2);
	for(uint32_t i = 0; i < c_initial_length; ++i)
	{
		SnakeSegment segment;
		segment.position[0] = c_field_width / 2;
		segment.position[1] = offset - i;
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
	bool hit_obstacle = false;
	switch(snake_->direction)
	{
	case GridDirection::XPlus:
		if(new_segment.position[0] < c_field_width - 1)
		{
			new_segment.position[0] += 1;
		}
		else
		{
			hit_obstacle = true;
		}
		break;
	case GridDirection::XMinus:
		if(new_segment.position[0] > 0)
		{
			new_segment.position[0] -= 1;
		}
		else
		{
			hit_obstacle = true;
		}
		break;
	case GridDirection::YPlus:
		if(new_segment.position[1] < c_field_height - 1)
		{
			new_segment.position[1] += 1;
		}
		else
		{
			hit_obstacle = true;
		}
		break;
	case GridDirection::YMinus:
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

	hit_obstacle |=
		tetris_field_[new_segment.position[0] + new_segment.position[1] * c_field_width] != TetrisBlock::Empty;

	if(tetris_active_piece_ != std::nullopt)
	{
		for(const TetrisPieceBlock& block : tetris_active_piece_->blocks)
		{
			hit_obstacle |=
				block[0] == int32_t(new_segment.position[0]) &&
				block[1] == int32_t(new_segment.position[1]);
		}
	}

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

			// Spawn Arkanoid balls on bonus pick-up.
			TrySpawnArkanoidBall();
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

void GameSnake::MoveSnakeAsTetrisPiece()
{
	if(snake_ == std::nullopt)
	{
		return;
	}

	bool can_move = true;
	for(const SnakeSegment& snake_segment : snake_->segments)
	{
		can_move &= snake_segment.position[1] + 1 < c_field_height;
	}

	if(can_move)
	{
		for(SnakeSegment& snake_segment : snake_->segments)
		{
			snake_segment.position[1] += 1;
		}
		sound_player_.PlaySound(SoundId::TetrisFigureStep);
	}
}

void GameSnake::ManipulateSnakeAsTetrisPiece(const std::vector<SDL_Event>& events)
{
	if (snake_ == std::nullopt)
	{
		return;
	}

	bool has_move_left = false;
	bool has_move_right = false;
	bool has_move_down = false;
	bool has_rotate = false;
	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN)
		{

			has_move_left |= event.key.keysym.scancode == SDL_SCANCODE_LEFT;
			has_move_right |= event.key.keysym.scancode == SDL_SCANCODE_RIGHT;
			has_move_down |= event.key.keysym.scancode == SDL_SCANCODE_DOWN;
			has_rotate |= event.key.keysym.scancode == SDL_SCANCODE_UP;
		}
	}

	const auto try_side_move_piece =
	[&](const int32_t delta)
	{
		bool can_move = true;
		for(const SnakeSegment& snake_segment : snake_->segments)
		{
			const auto next_x = int32_t(snake_segment.position[0]) + delta;
			if( next_x < 0 || next_x >= int32_t(c_field_width ))
			{
				// Do not check for field blocks because in tetris transition there is no tetris blocks on the field.
				can_move = false;
			}
		}

		if(can_move)
		{
			for(SnakeSegment& snake_segment : snake_->segments)
			{
				snake_segment.position[0] += uint32_t(delta);
			}
		}
	};

	if(has_move_left)
	{
		try_side_move_piece(-1);
	}
	if(has_move_right)
	{
		try_side_move_piece(1);
	}

	if(has_move_down)
	{
		bool can_move = true;
		for(const SnakeSegment& snake_segment : snake_->segments)
		{
			can_move &= snake_segment.position[1] + 1 < c_field_height;
		}

		if(can_move)
		{
			for(SnakeSegment& snake_segment : snake_->segments)
			{
				snake_segment.position[1] += 1;
			}
			sound_player_.PlaySound(SoundId::TetrisFigureStep);
		}
	}

	if(has_rotate)
	{
		std::vector<SnakeSegment> segments_transformed;
		segments_transformed.reserve(snake_->segments.size());
		bool can_rotate = true;

		const std::array<int32_t, 2> center = {
			int32_t(snake_->segments[snake_->segments.size() / 2].position[0]),
			int32_t(snake_->segments[snake_->segments.size() / 2].position[1])};
		for(const SnakeSegment& segment : snake_->segments)
		{
			const int32_t rel_x = int32_t(segment.position[0]) - center[0];
			const int32_t rel_y = int32_t(segment.position[1]) - center[1];
			const int32_t new_x = center[0] + rel_y;
			const int32_t new_y = center[1] - rel_x;

			if(new_x < 0 || new_y < 0 || new_x >= int32_t(c_field_width) || new_y >= int32_t(c_field_height))
			{
				can_rotate = false;
				break;
			}

			SnakeSegment segment_transformed;
			segment_transformed.position = {uint32_t(new_x), uint32_t(new_y)};
			segments_transformed.push_back(segment_transformed);
		}

		if(can_rotate)
		{
			GridDirection new_direction = snake_->direction;
			switch(snake_->direction)
			{
			case GridDirection::XPlus:
				new_direction = GridDirection::YMinus;
				break;
			case GridDirection::XMinus:
				new_direction = GridDirection::YPlus;
				break;
			case GridDirection::YPlus:
				new_direction = GridDirection::XPlus;
				break;
			case GridDirection::YMinus:
				new_direction = GridDirection::XMinus;
				break;
			}
			snake_->direction = new_direction;
			snake_->segments = segments_transformed;
		}
	}
}

void GameSnake::OnSnakeDeath()
{
	death_animation_end_tick_ = tick_ + g_death_animation_duration;
	sound_player_.PlaySound(SoundId::CharacterDeath);
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

	bool can_move = true;
	for(const TetrisPieceBlock& block : tetris_active_piece_->blocks)
	{
		assert(block[0] >= 0 && block[0] < int32_t(c_field_width));
		if(block[1] >= int32_t(c_field_height - 1) ||
			(block[1] >= 0 && block[1] + 1 < int32_t(c_field_height) &&
			tetris_field_[uint32_t(block[0]) + uint32_t(block[1] + 1) * c_field_width] != TetrisBlock::Empty))
		{
			can_move = false;
		}
	}

	if(can_move)
	{
		for(TetrisPieceBlock& block : tetris_active_piece_->blocks)
		{
			block[1] += 1;
		}
	}
	else
	{
		for(const TetrisPieceBlock& block : tetris_active_piece_->blocks)
		{
			assert(block[0] >= 0 && block[0] < int32_t(c_field_width ));
			assert(block[1] >= 0 && block[1] < int32_t(c_field_height));
			TetrisBlock& dst_block = tetris_field_[uint32_t(block[0]) + uint32_t(block[1]) * c_field_width];
			assert(dst_block == TetrisBlock::Empty);
			dst_block = tetris_active_piece_->type;

			// Respawn bonus if it lies behin the block.
			for(Bonus& bonus : bonuses_)
			{
				if(int32_t(bonus.position[0]) == block[0] && int32_t(bonus.position[1]) == block[1])
				{
					bonus = SpawnBonus();
				}
			}
		}
		tetris_active_piece_ = std::nullopt;
	}
}

bool GameSnake::UpdateArkanoidBall(ArkanoidBall& arkanoid_ball)
{
	arkanoid_ball.position[0] += arkanoid_ball.velocity[0];
	arkanoid_ball.position[1] += arkanoid_ball.velocity[1];

	const fixed16_t ball_half_size = IntToFixed16(3) / 20;

	// Bounce ball from tetris blocks.
	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		TetrisBlock& block = tetris_field_[x + y * c_field_width];
		if(block == TetrisBlock::Empty)
		{
			continue;
		}

		if(MakeCollisionBetweenObjectAndBox(
			{
				IntToFixed16(int32_t(x)),
				IntToFixed16(int32_t(y)),
			},
			{
				IntToFixed16(int32_t(x + 1)),
				IntToFixed16(int32_t(y + 1)),
			},
			{ball_half_size, ball_half_size},
			arkanoid_ball.position,
			arkanoid_ball.velocity))
		{
			block = TetrisBlock::Empty;
			sound_player_.PlaySound(SoundId::ArkanoidBallHit);

			assert(arkanoid_ball.bounces_left > 0);
			--arkanoid_ball.bounces_left;
			if(arkanoid_ball.bounces_left == 0)
			{
				return true;
			}
		}
	}

	// Bounse ball from snake segments.
	if(snake_ != std::nullopt)
	{
		bool hit = false;
		for(const SnakeSegment& segment : snake_->segments)
		{
			if(MakeCollisionBetweenObjectAndBox(
				{
					IntToFixed16(int32_t(segment.position[0])),
					IntToFixed16(int32_t(segment.position[1])),
				},
				{
					IntToFixed16(int32_t(segment.position[0] + 1)),
					IntToFixed16(int32_t(segment.position[1] + 1)),
				},
				{ball_half_size, ball_half_size},
				arkanoid_ball.position,
				arkanoid_ball.velocity))
			{
				hit = true;
				break;
			}
		}

		if(hit)
		{
			// Reduce snake size on hit.
			// TODO - maybe reduce size bu multiple segments?
			if(snake_->segments.size() <= size_t(g_min_snake_len))
			{
				OnSnakeDeath();
			}
			snake_->segments.pop_back();

			sound_player_.PlaySound(SoundId::ArkanoidBallHit);

			assert(arkanoid_ball.bounces_left > 0);
			--arkanoid_ball.bounces_left;
			if(arkanoid_ball.bounces_left == 0)
			{
				return true;
			}
		}
	}

	// Bounce ball from walls.
	// Do this only after blocks bouncing to make sure that ball is inside game field.
	const fixed16vec2_t filed_mins =
	{
		ball_half_size,
		ball_half_size,
	};
	const fixed16vec2_t filed_maxs =
	{
		IntToFixed16(c_field_width ) - ball_half_size,
		IntToFixed16(c_field_height) - ball_half_size,
	};

	for(size_t i = 0; i < 2; ++i)
	{
		if(arkanoid_ball.position[i] < filed_mins[i])
		{
			arkanoid_ball.velocity[i] = -arkanoid_ball.velocity[i];
			arkanoid_ball.position[i] = 2 * filed_mins[i] - arkanoid_ball.position[i];
			assert(arkanoid_ball.position[i] >= filed_mins[i]);

			sound_player_.PlaySound(SoundId::ArkanoidBallHit);

			assert(arkanoid_ball.bounces_left > 0);
			--arkanoid_ball.bounces_left;
			if(arkanoid_ball.bounces_left == 0)
			{
				return true;
			}
		}
		if(arkanoid_ball.position[i] > filed_maxs[i])
		{
			arkanoid_ball.velocity[i] = -arkanoid_ball.velocity[i];
			arkanoid_ball.position[i] = 2 * filed_maxs[i] - arkanoid_ball.position[i];
			assert(arkanoid_ball.position[i] <= filed_maxs[i]);

			sound_player_.PlaySound(SoundId::ArkanoidBallHit);

			assert(arkanoid_ball.bounces_left > 0);
			--arkanoid_ball.bounces_left;
			if(arkanoid_ball.bounces_left == 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool GameSnake::IsPositionFree(const std::array<uint32_t, 2>& position) const
{
	// Consider positions near tetris blocks non-free.
	for(int32_t dy = -1; dy <= 1; ++dy)
	{
		const int32_t y = dy + int32_t(position[1]);
		if(y < 0 || y >= int32_t(c_field_height))
		{
			continue;
		}
		for(int32_t dx = -1; dx <= 1; ++dx)
		{
			const int32_t x = dx + int32_t(position[0]);
			if(x < 0 || x >= int32_t(c_field_width))
			{
				continue;
			}
			if(tetris_field_[uint32_t(x) + uint32_t(y) * c_field_width] != TetrisBlock::Empty)
			{
				return false;
			}
		}
	}

	if(tetris_active_piece_ != std::nullopt)
	{
		for(const TetrisPieceBlock& block : tetris_active_piece_->blocks)
		{
			if(int32_t(position[0]) == block[0] && int32_t(position[1]) == block[1])
			{
				return false;
			}
		}
	}

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
	if(tick_ < g_transition_time_change_end)
	{
		return;
	}
	if(tetris_active_piece_ != std::nullopt)
	{
		return;
	}
	if(death_animation_end_tick_ != std::nullopt ||
		field_start_animation_end_tick_ != std::nullopt || level_end_animation_end_tick_ != std::nullopt)
	{
		return;
	}

	if(rand_.Next() % (7 * GameInterface::c_update_frequency) != 17)
	{
		return;
	}

	const TetrisBlock type = TetrisBlock(uint32_t(TetrisBlock::I) + rand_.Next() % g_tetris_num_piece_types);
	TetrisPieceBlocks blocks = g_tetris_pieces_blocks[uint32_t(type) - uint32_t(TetrisBlock::I)];

	// Move piece down to spawn in visible state.
	// This helps player to turn snake to avoid collision.
	for(TetrisPieceBlock& block : blocks)
	{
		block[1] += 1;
	}

	// Choose random rotation.
	const uint32_t num_rotations = rand_.Next() / 47u % 4u;
	for(uint32_t i = 0; i < num_rotations; ++i)
	{
		TetrisPiece piece;
		piece.type = type;
		piece.blocks = blocks;
		blocks = RotateTetrisPieceBlocks(piece);
	}

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
		assert(cur_min_x >= 0);
		assert(cur_min_x <= cur_max_x);
		assert(cur_max_x < int32_t(c_field_width));

		if(snake_ != std::nullopt)
		{
			bool may_hit_snake = false;
			for(const SnakeSegment& segment : snake_->segments)
			{
				if(int32_t(segment.position[0]) + 1 >= cur_min_x && int32_t(segment.position[0]) - 1 <= cur_max_x)
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

		// Keep upper 4 rows free.
		{
			bool may_hit_block = false;

			for(uint32_t y = 0; y < 4; ++y)
			for(uint32_t x = uint32_t(cur_min_x); x <= uint32_t(cur_max_x); ++x)
			{
				if(tetris_field_[x + y * c_field_width] != TetrisBlock::Empty)
				{
					may_hit_block = true;
					break;
				}
			}

			if(may_hit_block)
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

void GameSnake::TrySpawnArkanoidBall()
{
	if(tick_ < g_transition_time_change_end)
	{
		return;
	}
	if(rand_.Next() % 7 != 0)
	{
		return;
	}

	const fixed16_t speed = g_fixed16_one / 10;

	ArkanoidBall ball;

	ball.bounces_left = 7;

	const float random_angle = rand_.RandomAngle();
	ball.velocity[0] = fixed16_t(std::cos(random_angle) * float(speed));
	ball.velocity[1] = fixed16_t(std::sin(random_angle) * float(speed));

	// Try to select random position far enough from the snake and any tetris block.
	for(uint32_t i = 0; i < 200; ++i)
	{
		ball.position[0] = fixed16_t(rand_.Next() % (c_field_width  << g_fixed16_base));
		ball.position[1] = fixed16_t(rand_.Next() % (c_field_height << g_fixed16_base));

		const int32_t ball_x = Fixed16FloorToInt(ball.position[0]);
		const int32_t ball_y = Fixed16FloorToInt(ball.position[1]);

		bool can_place = true;
		for(int32_t dy = -2; dy <= 2; ++dy)
		{
			const int32_t y = ball_y + dy;
			if(y < 0 || y >= int32_t(c_field_height))
			{
				continue;
			}
			for(int32_t dx = -2; dx <= 2; ++dx)
			{
				const int32_t x = ball_x + dx;
				if(x < 0 || x >= int32_t(c_field_width))
				{
					continue;
				}
				can_place &=
					tetris_field_[uint32_t(x) + uint32_t(y) * c_field_width] == TetrisBlock::Empty;
			}
		}

		if(snake_ != std::nullopt)
		{
			const int32_t min_dist = 3;
			for(const SnakeSegment& segment : snake_->segments)
			{
				const int32_t dx = int32_t(segment.position[0]) - ball_x;
				const int32_t dy = int32_t(segment.position[1]) - ball_y;
				if(dx * dx + dy * dy < min_dist * min_dist)
				{
					can_place = false;
					break;
				}
			}
		}

		if(can_place)
		{
			arkanoid_balls_.push_back(ball);
			return;
		}
	} // for tries.
}
