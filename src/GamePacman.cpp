#include "GamePacman.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "GamesCommon.hpp"
#include "GamesDrawCommon.hpp"
#include "Progress.hpp"
#include "Sprites.hpp"
#include "Strings.hpp"
#include <cassert>

namespace
{

constexpr char g_game_field[]=
"                # #              "
" ############   # #   ########## "
" #....##@...#   # #   #.....@..# "
" #.##.##.##.#   # #   #.##.###.# "
" #.##....##.#   # #   #.##.###.# "
" #.##.#####.#   # #   #.##.###.# "
" #.##.#####.##### #####.##.###.# "
" #.##..........................# "
" #.#####.##.#####.########.###.# "
" #.#####.##.#####.########.###.# "
" #.##....##..........##....###.# "
" #.##.##.##.##.#####.##.##.###.# "
" #.##.##.##.##.#   #.##.##.###.# "
" #....##....##.#   #....##.....# "
" #.#####.#####.#    .#####.##### "
" #.#####.#####.#    .#####.##### "
" #....##....##.#   #....##.....# "
" #.##.##.##.##.#   #.##.##.###.# "
" #.##.##.##.##.#####.##.##.###.# "
" #.##....##..........##....###.# "
" #.#####.##.#####.########.###.# "
" #.#####.##.#####.########.###.# "
" #.##..........................# "
" #.##.#####.##### #####.##.###.# "
" #.##.#####.#   # #   #.##.###.# "
" #.##....##.#   # #   #.##.###.# "
" #.##.##.##.#   # #   #.##.###.# "
" #....##@...#   # #   #.....@..# "
" ############   # #   ########## "
"                # #              "
;

constexpr char g_wall_symbol = '#';
constexpr char g_food_symbol = '.';
constexpr char g_bonus_deadly_symbol = '@';

// See https://www.gamedeveloper.com/design/the-pac-man-dossier.
const fixed16_t g_base_move_speed = g_fixed16_one * 6 / GameInterface::c_update_frequency;
const fixed16_t g_pacman_move_speed = g_base_move_speed * 80 / 100;
const fixed16_t g_ghost_move_speed = g_base_move_speed * 75 / 100;
const fixed16_t g_ghost_frightened_move_speed = g_base_move_speed * 50 / 100;
const fixed16_t g_ghost_eaten_move_speed = g_base_move_speed * 2;
const fixed16_t g_laser_beam_speed = g_base_move_speed * 400 / 100;

const fixed16_t g_character_half_size = g_fixed16_one + 1;

const uint32_t g_frightened_mode_duration = GameInterface::c_update_frequency * 10;
const uint32_t g_death_animation_duration = GameInterface::c_update_frequency * 3 / 2;
const uint32_t g_spawn_animation_duration = GameInterface::c_update_frequency * 3;
const uint32_t g_min_shoot_interval = 45;

const uint32_t g_scatter_duration_first = 7 * GameInterface::c_update_frequency * 3 / 2;
const uint32_t g_scatter_duration_second = 5 * GameInterface::c_update_frequency * 3 / 2;
const uint32_t g_chase_duration = 20 * GameInterface::c_update_frequency * 3 / 2;

const uint32_t g_arkanoid_ball_mode_duration = 15 * GameInterface::c_update_frequency;

const uint32_t g_bonuses_eaten_for_pinky_release = 0;
const uint32_t g_bonuses_eaten_for_inky_release = g_bonuses_eaten_for_pinky_release + 30;
const uint32_t g_bonuses_eaten_for_clyde_release = g_bonuses_eaten_for_inky_release + 45;

const uint32_t g_max_lifes = 8;

const uint32_t g_score_for_food = 10;
const uint32_t g_score_for_deadly_bonus = 30;
const uint32_t g_score_for_snake_bonus = 30;
const uint32_t g_score_for_ghost = 200;

const uint32_t g_transition_snake_move_speed = 60;
const uint32_t g_transition_time_change_snake = g_transition_snake_move_speed * 22 / 2;
const uint32_t g_transition_time_hide_snake_stats = g_transition_time_change_snake * 1 / 3;
const uint32_t g_transition_time_change_field_border = g_transition_time_change_snake * 2 / 3;
const uint32_t g_transition_time_show_pacman_field = g_transition_time_change_snake + GameInterface::c_update_frequency / 2;
const uint32_t g_transition_time_show_pacman_stats = g_transition_time_show_pacman_field + GameInterface::c_update_frequency / 2;

const uint32_t g_transition_time_change_end = g_transition_time_show_pacman_field;

const SpriteBMP g_bonus_sprites[]
{
	Sprites::pacman_food,
	Sprites::pacman_food,
	Sprites::pacman_bonus_deadly,
	Sprites::tetris_block_small_4,
	Sprites::tetris_block_small_7,
	Sprites::tetris_block_small_5,
	Sprites::tetris_block_small_1,
	Sprites::tetris_block_small_2,
	Sprites::tetris_block_small_6,
	Sprites::tetris_block_small_3,
	Sprites::snake_food_small,
	Sprites::snake_food_medium,
	Sprites::snake_food_large,
	Sprites::snake_extra_life,
};

} // namespace

GamePacman::GamePacman(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
	OpenGame(GameId::Pacman);

	NextLevel();

	//spawn_animation_end_tick_ = g_transition_time_change_end;
	//assert(g_transition_time_change_end >= g_spawn_animation_duration);
	//next_ghosts_mode_swith_tick_ += g_transition_time_change_end - g_spawn_animation_duration;
	spawn_animation_end_tick_ = g_transition_time_change_snake;

	temp_snake_position_ = {IntToFixed16(2) + g_fixed16_one / 2, IntToFixed16(5) + g_fixed16_one / 2};

	const fixed16_t bonus_x = IntToFixed16(2) + g_fixed16_one / 2;
	const fixed16_t bonus_step = g_fixed16_one * 10 / 8;
	snake_transition_bonuses_.push_back(
		{{bonus_x, temp_snake_position_[1] + bonus_step * 2}, Bonus::SnakeFoodSmall});
	snake_transition_bonuses_.push_back(
		{{bonus_x, temp_snake_position_[1] + bonus_step * 5}, Bonus::SnakeFoodMedium});
	snake_transition_bonuses_.push_back(
		{{bonus_x, temp_snake_position_[1] + bonus_step * 7}, Bonus::SnakeFoodLarge});
	snake_transition_bonuses_.push_back(
		{{bonus_x, temp_snake_position_[1] + bonus_step * 8}, Bonus::Food});
	snake_transition_bonuses_.push_back(
		{{bonus_x, temp_snake_position_[1] + bonus_step * 9}, Bonus::Food});
	snake_transition_bonuses_.push_back(
		{{bonus_x, temp_snake_position_[1] + bonus_step * 10}, Bonus::Food});
}

void GamePacman::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void)keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
		if(event.type == SDL_KEYDOWN && pacman_.dead_animation_end_tick == std::nullopt)
		{
			if(event.key.keysym.scancode == SDL_SCANCODE_LEFT)
			{
				pacman_.next_direction = GridDirection::XMinus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
			{
				pacman_.next_direction = GridDirection::XPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_DOWN)
			{
				pacman_.next_direction = GridDirection::YPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_UP)
			{
				pacman_.next_direction = GridDirection::YMinus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_RCTRL ||
				event.key.keysym.scancode == SDL_SCANCODE_LCTRL ||
				event.key.keysym.scancode == SDL_SCANCODE_SPACE)
			{
				ProcessShootRequest();
			}
		}
		if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 1)
		{
			ProcessShootRequest();
		}
	}

	++tick_;

	UpdateGhostsMode();

	UpdateSnakePosition();
	MovePacman();

	if(tick_ >= g_transition_time_change_end)
	{
		for(Ghost& ghost : ghosts_)
		{
			MoveGhost(ghost);
		}
	}

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

	ProcessPacmanGhostsTouch();
	TryTeleportCharacters();

	if(!game_over_ && pacman_.dead_animation_end_tick != std::nullopt && tick_ > *pacman_.dead_animation_end_tick)
	{
		if(lifes_ > 0)
		{
			--lifes_;
			SpawnPacmanAndGhosts();
		}
		else
		{
			game_over_ = true;
		}
	}

	if(bonuses_left_ == 0)
	{
		NextLevel();
	}
}

void GamePacman::Draw(const FrameBuffer frame_buffer) const
{
	static_assert(std::size(g_game_field) == c_field_width * c_field_height + 1, "Wrong field size");

	FillWholeFrameBuffer(frame_buffer, g_color_black);

	if(tick_ < g_transition_time_hide_snake_stats)
	{
		DrawSnakeStats(frame_buffer, 4, lifes_, level_, score_);
	}

	const SpriteBMP snake_border_sprite( Sprites::snake_field_border);
	if(tick_ < g_transition_time_change_field_border)
	{
		for(uint32_t x = 0; x < frame_buffer.width / snake_border_sprite.GetWidth(); ++x)
		{
			DrawSprite(frame_buffer, snake_border_sprite, x * snake_border_sprite.GetWidth(), 0);
			DrawSprite(frame_buffer, snake_border_sprite, x * snake_border_sprite.GetWidth(), frame_buffer.height - 3 * snake_border_sprite.GetHeight());
		}
		for(uint32_t y = 0; y < frame_buffer.height / snake_border_sprite.GetHeight() - 3; ++y)
		{
			DrawSprite(frame_buffer, snake_border_sprite, 0, y * snake_border_sprite.GetHeight());
			DrawSprite(frame_buffer, snake_border_sprite, frame_buffer.width - snake_border_sprite.GetWidth(), y * snake_border_sprite.GetHeight());
		}

	}
	else if(tick_ < g_transition_time_show_pacman_field)
	{
		for(uint32_t x = 0; x < 26; ++x)
		{
			DrawSprite(frame_buffer, snake_border_sprite, x * snake_border_sprite.GetWidth(), 0);
			DrawSprite(frame_buffer, snake_border_sprite, x * snake_border_sprite.GetWidth(), 230);
		}
		for(uint32_t y = 0; y < 24; ++y)
		{
			DrawSprite(frame_buffer, snake_border_sprite, 0, y * snake_border_sprite.GetHeight());
			DrawSprite(frame_buffer, snake_border_sprite, 254, y * snake_border_sprite.GetHeight());
		}
	}
	else
	{
		DrawBonuses(frame_buffer);
		DrawField(frame_buffer);
	}

	if(tick_ < g_transition_time_change_snake && !snake_transition_bonuses_.empty())
	{
		const SnakeTransitionBonus& bonus = snake_transition_bonuses_.front();
		const SpriteBMP sprite = g_bonus_sprites[uint32_t(bonus.type)];
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			uint32_t(Fixed16FloorToInt(bonus.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			uint32_t(Fixed16FloorToInt(bonus.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}

	if(tick_ >= g_transition_time_change_end)
	{
		for(const Ghost& ghost : ghosts_)
		{
			if(ghost.mode == GhostMode::Frightened || ghost.mode == GhostMode::Eaten)
			{
				DrawGhost(frame_buffer, ghost);
			}
		}
	}

	if(tick_ < g_transition_time_change_snake)
	{
		const uint32_t snake_segment_size = 10;
		const uint32_t half_snake_segment_size = snake_segment_size / 2;
		const uint32_t num_snake_segments = 4;
		const uint32_t offset_x = uint32_t(Fixed16FloorToInt(temp_snake_position_[0] * int32_t(c_block_size))) - half_snake_segment_size;
		const uint32_t offset_y = uint32_t(Fixed16FloorToInt(temp_snake_position_[1] * int32_t(c_block_size))) - half_snake_segment_size - (num_snake_segments - 1) * snake_segment_size;
		DrawSpriteWithAlpha(frame_buffer, Sprites::snake_tail, 0, offset_x, offset_y);
		for(uint32_t i = 1; i + 1 < num_snake_segments; ++i)
		{
			DrawSpriteWithAlpha(frame_buffer, Sprites::snake_body_segment, 0, offset_x, offset_y + snake_segment_size * i);
		}
		DrawSpriteWithAlpha(frame_buffer, Sprites::snake_head, 0, offset_x, offset_y + snake_segment_size * (num_snake_segments - 1));
	}
	else
	{
		DrawPacman(frame_buffer);
	}

	if(tick_ >= g_transition_time_change_end)
	{
		for(const Ghost& ghost : ghosts_)
		{
			if(!(ghost.mode == GhostMode::Frightened || ghost.mode == GhostMode::Eaten))
			{
				DrawGhost(frame_buffer, ghost);
			}
		}
	}

	for(const LaserBeam& laser_beam : laser_beams_)
	{
		const SpriteBMP sprite(Sprites::arkanoid_laser_beam);
		const uint32_t x = uint32_t(Fixed16FloorToInt(laser_beam.position[0] * int32_t(c_block_size)));
		const uint32_t y = uint32_t(Fixed16FloorToInt(laser_beam.position[1] * int32_t(c_block_size)));
		const uint32_t half_height = sprite.GetHeight() / 2;
		switch(laser_beam.direction)
		{
		case GridDirection::XMinus:
			DrawSpriteWithAlphaRotate270(frame_buffer, sprite, 0, x - half_height, y - 2);
			DrawSpriteWithAlphaRotate270(frame_buffer, sprite, 0, x - half_height, y + 1);
			break;
		case GridDirection::XPlus:
			DrawSpriteWithAlphaRotate90 (frame_buffer, sprite, 0, x - half_height, y - 2);
			DrawSpriteWithAlphaRotate90 (frame_buffer, sprite, 0, x - half_height, y + 1);
			break;
		case GridDirection::YMinus:
			DrawSpriteWithAlphaRotate180(frame_buffer, sprite, 0, x - 2, y - half_height);
			DrawSpriteWithAlphaRotate180(frame_buffer, sprite, 0, x + 1, y - half_height);
			break;
		case GridDirection::YPlus:
			DrawSpriteWithAlpha         (frame_buffer, sprite, 0, x - 2, y - half_height);
			DrawSpriteWithAlpha         (frame_buffer, sprite, 0, x + 1, y - half_height);
			break;
		}
	}

	if(tick_ >= g_transition_time_show_pacman_stats)
	{
		const SpriteBMP life_spirte(Sprites::pacman_life);
		for(uint32_t i = 0; i < lifes_; ++i)
		{
			DrawSpriteWithAlpha(
				frame_buffer,
				life_spirte,
				0,
				c_field_width * c_block_size - c_block_size / 2 + i % 4 * (life_spirte.GetWidth() + 2),
				c_block_size * 2 + i / 4 * (life_spirte.GetHeight() + 3));
		}

		const uint32_t texts_offset_x = c_field_width * c_block_size - c_block_size / 2;
		const uint32_t texts_offset_y = 8 * g_glyph_height;

		char text[64];

		DrawText(frame_buffer, g_cga_palette[10], texts_offset_x, texts_offset_y + 0 * g_glyph_height, Strings::pacman_level);

		NumToString(text, sizeof(text), level_, 7);
		DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y + 2 * g_glyph_height, text);

		DrawText(frame_buffer, g_cga_palette[10], texts_offset_x, texts_offset_y + 5 * g_glyph_height, Strings::pacman_score);

		NumToString(text, sizeof(text), score_, 7);
		DrawText(frame_buffer, g_color_white, texts_offset_x, texts_offset_y + 7 * g_glyph_height, text);

		if(tick_ < spawn_animation_end_tick_)
		{
			DrawTextCentered(
				frame_buffer,
				g_cga_palette[9 + tick_ / 16 % 7],
				c_field_width  * c_block_size / 2,
				c_field_height * c_block_size / 2,
				Strings::pacman_ready);
		}
		if(game_over_)
		{
			if(tick_ / 16 % 2 != 0)
			{
				DrawTextCentered(
					frame_buffer,
					g_cga_palette[14],
					c_field_width  * c_block_size / 2,
					c_field_height * c_block_size / 2,
					Strings::pacman_game_over);
			}
		}
	}
}

GameInterfacePtr GamePacman::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GamePacman::DrawField(const FrameBuffer frame_buffer) const
{
	const Color32 c_wall_color = g_cga_palette[1];
	for(uint32_t y = 0; y < c_field_height; ++y)
	{
		const char* const line = g_game_field + y * c_field_width;
		const char* const line_y_minus = g_game_field + (std::max(1u, y) - 1) * c_field_width;
		const char* const line_y_plus  = g_game_field + (std::min(c_field_height - 2, y) + 1) * c_field_width;

		for(uint32_t x = 0; x < c_field_width ; ++x)
		{
			const char block = line[x];
			if(block != g_wall_symbol)
			{
				continue;
			}

			const uint32_t block_x = x * c_block_size;
			const uint32_t block_y = y * c_block_size;
			const auto set_pixel = [&](const uint32_t dx, const uint32_t dy)
			{
				frame_buffer.data[ (block_x + dx) + (block_y + dy) * frame_buffer.width] = c_wall_color;
			};

			const uint32_t x_minus_one_clamped = std::max(x, 1u) - 1;
			const uint32_t x_plus_one_clamped = std::min(x, c_field_width - 2) + 1;

			const bool block_y_minus = line_y_minus[x] == g_wall_symbol;
			const bool block_y_plus  = line_y_plus [x] == g_wall_symbol;
			const bool block_x_minus = line[x_minus_one_clamped] == g_wall_symbol;
			const bool block_x_plus  = line[x_plus_one_clamped ] == g_wall_symbol;
			const bool block_x_minus_y_minus = line_y_minus[x_minus_one_clamped] == g_wall_symbol;
			const bool block_x_minus_y_plus  = line_y_plus [x_minus_one_clamped] == g_wall_symbol;
			const bool block_x_plus_y_minus  = line_y_minus[x_plus_one_clamped ] == g_wall_symbol;
			const bool block_x_plus_y_plus   = line_y_plus [x_plus_one_clamped ] == g_wall_symbol;

			// Sides.
			if((block_x_plus && block_x_minus) || (!block_y_minus && !block_y_plus))
			{
				if(!block_y_minus)
				{
					for(uint32_t dx = 0; dx < c_block_size; ++dx)
						set_pixel(dx, 4);
				}
				if(!block_y_plus )
				{
					for(uint32_t dx = 0; dx < c_block_size; ++dx)
						set_pixel(dx, 3);
				}
			}
			if(block_x_plus && block_x_minus)
			{
				if(!block_y_minus && (!block_x_plus_y_plus   || !block_x_minus_y_plus ))
				{
					for(uint32_t dx = 0; dx < c_block_size; ++dx)
						set_pixel(dx, 3);
				}
				if(!block_y_plus  && (!block_x_plus_y_minus  || !block_x_minus_y_minus))
				{
					for(uint32_t dx = 0; dx < c_block_size; ++dx)
						set_pixel(dx, 4);
				}
			}
			if((block_y_minus && block_y_plus) || (!block_x_minus && !block_x_plus))
			{
				if(!block_x_minus)
				{
					for(uint32_t dy = 0; dy < c_block_size; ++dy)
						set_pixel(4, dy);
				}
				if(!block_x_plus)
				{
					for(uint32_t dy = 0; dy < c_block_size; ++dy)
						set_pixel(3, dy);
				}
			}
			if(block_y_plus && block_y_minus)
			{
				if(!block_x_minus && (!block_x_plus_y_minus  || !block_x_plus_y_plus  ))
				{
					for(uint32_t dy = 0; dy < c_block_size; ++dy)
						set_pixel(3, dy);
				}
				if(!block_x_plus && (!block_x_minus_y_minus  || !block_x_minus_y_plus ))
				{
					for(uint32_t dy = 0; dy < c_block_size; ++dy)
						set_pixel(4, dy);
				}
			}

			// Outer corners.
			if(!block_x_minus && !block_y_minus && block_x_plus && block_y_plus)
			{
				set_pixel(4, 6);
				set_pixel(4, 7);
				set_pixel(6, 4);
				set_pixel(7, 4);
				set_pixel(5, 5);
			}
			if(!block_x_plus && !block_y_minus && block_x_minus && block_y_plus)
			{
				set_pixel(3, 6);
				set_pixel(3, 7);
				set_pixel(0, 4);
				set_pixel(1, 4);
				set_pixel(2, 5);
			}
			if(!block_x_minus && !block_y_plus  && block_x_plus  && block_y_minus)
			{
				set_pixel(4, 0);
				set_pixel(4, 1);
				set_pixel(6, 3);
				set_pixel(7, 3);
				set_pixel(5, 2);
			}
			if(!block_x_plus && !block_y_plus  && block_x_minus && block_y_minus)
			{
				set_pixel(3, 0);
				set_pixel(3, 1);
				set_pixel(0, 3);
				set_pixel(1, 3);
				set_pixel(2, 2);
			}

			// Inner corners.
			if(block_x_minus && block_y_minus && !block_x_minus_y_minus)
			{
				set_pixel(4, 0);
				set_pixel(4, 1);
				set_pixel(4, 2);
				set_pixel(0, 4);
				set_pixel(1, 4);
				set_pixel(2, 4);
				set_pixel(3, 3);
			}
			if(block_x_minus && block_y_plus && !block_x_minus_y_plus )
			{
				set_pixel(4, 5);
				set_pixel(4, 6);
				set_pixel(4, 7);
				set_pixel(0, 3);
				set_pixel(1, 3);
				set_pixel(2, 3);
				set_pixel(3, 4);
			}
			if(block_x_plus && block_y_minus && !block_x_plus_y_minus  )
			{
				set_pixel(3, 0);
				set_pixel(3, 1);
				set_pixel(3, 2);
				set_pixel(5, 4);
				set_pixel(6, 4);
				set_pixel(7, 4);
				set_pixel(4, 3);
			}
			if(block_x_plus && block_y_plus  && !block_x_plus_y_plus  )
			{
				set_pixel(3, 5);
				set_pixel(3, 6);
				set_pixel(3, 7);
				set_pixel(5, 3);
				set_pixel(6, 3);
				set_pixel(7, 3);
				set_pixel(4, 4);
			}
		} // for x
	} // for y
}

void GamePacman::DrawBonuses(const FrameBuffer frame_buffer) const
{
	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		const Bonus bonus = bonuses_[x + y * c_field_width];
		if(bonus == Bonus::None)
		{
			continue;
		}

		const SpriteBMP sprite = g_bonus_sprites[uint32_t(bonus)];
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			x * c_block_size + c_block_size / 2 - sprite.GetWidth () / 2,
			y * c_block_size + c_block_size / 2 - sprite.GetHeight() / 2);
	}
}

void GamePacman::DrawPacman(const FrameBuffer frame_buffer) const
{
	if(pacman_.arkanoid_ball != std::nullopt)
	{
		const SpriteBMP sprite(Sprites::arkanoid_ball);
		DrawSpriteWithAlpha(
			frame_buffer,
			sprite,
			0,
			uint32_t(Fixed16FloorToInt(pacman_.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
			uint32_t(Fixed16FloorToInt(pacman_.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
	}
	else if(pacman_.dead_animation_end_tick != std::nullopt)
	{
		const SpriteBMP sprites[]
		{
			Sprites::pacman_0,
			Sprites::pacman_1,
			Sprites::pacman_2,
			Sprites::pacman_3,
			Sprites::pacman_4,
			Sprites::pacman_5,
			Sprites::pacman_6,
			Sprites::pacman_7,
		};

		const uint32_t num_frames = uint32_t(std::size(sprites));

		const uint32_t frame =
			(g_death_animation_duration + tick_ - *pacman_.dead_animation_end_tick) *
			num_frames /
			g_death_animation_duration;

		const SpriteBMP current_sprite = sprites[std::min(frame, num_frames - 1)];
		const uint32_t pacman_x =
			uint32_t(Fixed16FloorToInt(pacman_.position[0] * int32_t(c_block_size))) - current_sprite.GetWidth () / 2;
		const uint32_t pacman_y =
			uint32_t(Fixed16FloorToInt(pacman_.position[1] * int32_t(c_block_size))) - current_sprite.GetHeight() / 2;
		switch(pacman_.direction)
		{
		case GridDirection::XMinus:
			DrawSpriteWithAlphaRotate180(frame_buffer, current_sprite, 0, pacman_x, pacman_y);
			break;
		case GridDirection::XPlus:
			DrawSpriteWithAlpha         (frame_buffer, current_sprite, 0, pacman_x, pacman_y);
			break;
		case GridDirection::YMinus:
			DrawSpriteWithAlphaRotate270(frame_buffer, current_sprite, 0, pacman_x, pacman_y);
			break;
		case GridDirection::YPlus:
			DrawSpriteWithAlphaRotate90 (frame_buffer, current_sprite, 0, pacman_x, pacman_y);
			break;
		}
	}
	else
	{
		const SpriteBMP sprites[]
		{
			Sprites::pacman_0,
			Sprites::pacman_1,
			Sprites::pacman_2,
			Sprites::pacman_3,
			Sprites::pacman_2,
			Sprites::pacman_1,
		};

		const uint32_t num_frames = uint32_t(std::size(sprites));

		const fixed16_t dist =
			Fixed16Abs(pacman_.target_position[0] - pacman_.position[0]) +
			Fixed16Abs(pacman_.target_position[1] - pacman_.position[1]);

		const uint32_t frame =
			std::min(
				(uint32_t(std::max(g_fixed16_one - dist, 0)) * num_frames) >> g_fixed16_base,
				num_frames - 1);

		const SpriteBMP current_sprite =
			tick_ < spawn_animation_end_tick_ ? sprites[1] : sprites[frame];
		const uint32_t pacman_x =
			uint32_t(Fixed16FloorToInt(pacman_.position[0] * int32_t(c_block_size))) - current_sprite.GetWidth() / 2;
		const uint32_t pacman_y =
			uint32_t(Fixed16FloorToInt(pacman_.position[1] * int32_t(c_block_size))) - current_sprite.GetHeight() / 2;

		auto func = DrawSpriteWithAlpha;
		switch(pacman_.direction)
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

		if(pacman_.turret_shots_left > 0)
		{
			func(frame_buffer, Sprites::pacman_turret, 0, pacman_x, pacman_y);
		}
		func(frame_buffer, current_sprite, 0, pacman_x, pacman_y);
	}
}

void GamePacman::DrawGhost(const FrameBuffer frame_buffer, const Ghost& ghost) const
{
	const SpriteBMP sprites[4][4]
	{
		{
			Sprites::pacman_ghost_0_right,
			Sprites::pacman_ghost_0_left ,
			Sprites::pacman_ghost_0_down ,
			Sprites::pacman_ghost_0_up   ,
		},
		{
			Sprites::pacman_ghost_1_right,
			Sprites::pacman_ghost_1_left ,
			Sprites::pacman_ghost_1_down ,
			Sprites::pacman_ghost_1_up   ,
		},
		{
			Sprites::pacman_ghost_2_right,
			Sprites::pacman_ghost_2_left ,
			Sprites::pacman_ghost_2_down ,
			Sprites::pacman_ghost_2_up   ,
		},
		{
			Sprites::pacman_ghost_3_right,
			Sprites::pacman_ghost_3_left ,
			Sprites::pacman_ghost_3_down ,
			Sprites::pacman_ghost_3_up   ,
		},
	};

	const SpriteBMP sprites_dead[4]
	{
		Sprites::pacman_ghost_dead_right,
		Sprites::pacman_ghost_dead_left ,
		Sprites::pacman_ghost_dead_down ,
		Sprites::pacman_ghost_dead_up   ,
	};

	SpriteBMP sprite = sprites[uint32_t(ghost.type)][uint32_t(ghost.direction)];
	if(ghost.mode == GhostMode::Eaten)
	{
		sprite = sprites_dead[uint32_t(ghost.direction)];
	}
	else if(ghost.mode == GhostMode::Frightened)
	{
		if(tick_ <= ghost.frightened_mode_end_tick)
		{
			const uint32_t time_left = ghost.frightened_mode_end_tick - tick_;
			const uint32_t flicker_duration = 2 * GameInterface::c_update_frequency;
			if(time_left >= flicker_duration || tick_ / 16 % 2 == 0)
			{
				sprite = Sprites::pacman_ghost_vulnerable;
			}
		}
	}

	DrawSpriteWithAlpha(
		frame_buffer,
		sprite,
		0,
		uint32_t(Fixed16FloorToInt(ghost.position[0] * int32_t(c_block_size))) - sprite.GetWidth () / 2,
		uint32_t(Fixed16FloorToInt(ghost.position[1] * int32_t(c_block_size))) - sprite.GetHeight() / 2);
}

void GamePacman::NextLevel()
{
	++level_;

	lifes_ = std::min(lifes_ + 2, g_max_lifes);

	bonuses_left_ = 0;
	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		const uint32_t address = x + y * c_field_width;
		const char symbol = g_game_field[address];
		Bonus& bonus = bonuses_[address];
		switch(symbol)
		{
		case g_food_symbol:
			bonus = Bonus::Food;
			++bonuses_left_;
			break;
		case g_bonus_deadly_symbol:
			bonus = Bonus::Deadly;
			++bonuses_left_;
			break;
		default:
			break;
		}
	}

	SpawnPacmanAndGhosts();
}

void GamePacman::SpawnPacmanAndGhosts()
{
	pacman_.target_position = {IntToFixed16(8) + g_fixed16_one / 2, IntToFixed16(12) + g_fixed16_one / 2};
	pacman_.position = pacman_.target_position;
	pacman_.direction = GridDirection::YPlus;
	pacman_.next_direction = pacman_.direction;
	pacman_.dead_animation_end_tick = std::nullopt;
	pacman_.turret_shots_left = 0;
	pacman_.next_shoot_tick = 0;
	pacman_.arkanoid_ball = std::nullopt;

	bonuses_eaten_ = 0;

	laser_beams_.clear();

	current_ghosts_mode_ = GhostMode::Scatter;
	ghosts_mode_switches_left_ = 4;
	next_ghosts_mode_swith_tick_ = tick_ + g_spawn_animation_duration + g_scatter_duration_first;

	for(uint32_t i = 0; i < c_num_ghosts; ++i)
	{
		Ghost& ghost = ghosts_[i];
		const uint32_t index_wrapped = i % 4;
		ghost.type = GhostType(index_wrapped % 4);
		if(ghost.type == GhostType::Blinky)
		{
			ghost.target_position = {IntToFixed16(20) + g_fixed16_one / 2, IntToFixed16(14) + g_fixed16_one / 2};
		}
		else
		{
			ghost.target_position = {
				IntToFixed16(17) + g_fixed16_one / 2,
				IntToFixed16(10 + int32_t(index_wrapped * 2)) + g_fixed16_one / 2};
		}
		ghost.position = ghost.target_position;

		ghost.mode = current_ghosts_mode_;
	}

	spawn_animation_end_tick_ = tick_ + g_spawn_animation_duration;
}

void GamePacman::ProcessShootRequest()
{
	if(pacman_.turret_shots_left == 0)
	{
		return;
	}
	if(tick_ < pacman_.next_shoot_tick)
	{
		return;
	}

	--pacman_.turret_shots_left;
	pacman_.next_shoot_tick = tick_ + g_min_shoot_interval;

	LaserBeam beam;
	beam.position = pacman_.position;
	beam.direction = pacman_.direction;

	laser_beams_.push_back(beam);
}

void GamePacman::MovePacman()
{
	if(pacman_.dead_animation_end_tick != std::nullopt || tick_ < spawn_animation_end_tick_)
	{
		return;
	}

	if(pacman_.arkanoid_ball != std::nullopt)
	{
		pacman_.position[0] += pacman_.arkanoid_ball->velocity[0];
		pacman_.position[1] += pacman_.arkanoid_ball->velocity[1];

		const int32_t x_center = Fixed16RoundToInt(pacman_.position[0]);
		const int32_t y_center = Fixed16RoundToInt(pacman_.position[1]);

		// Make ball very small in order to compensate wrong walls size.
		const fixed16_t ball_half_size = g_fixed16_one / 32;

		for(int32_t dy = -3; dy <= 3; ++dy)
		{
			const int32_t y = y_center + dy;
			if(y < 0 || y >= int32_t(c_field_height))
			{
				continue;
			}
			const fixed16_t dist_dy = pacman_.position[1] - (IntToFixed16(y) + g_fixed16_one / 2);

			for(int32_t dx = -3; dx <= 3; ++dx)
			{
				const int32_t x = x_center + dx;
				if(x < 0 || x >= int32_t(c_field_width))
				{
					continue;
				}

				const uint32_t address = uint32_t(x) + uint32_t(y) * c_field_width;

				const fixed16_t dist_dx = pacman_.position[0] - (IntToFixed16(x) + g_fixed16_one / 2);
				if(Fixed16VecSquareLen({dist_dx, dist_dy}) <= g_fixed16_one / 4)
				{
					PickUpBonus(bonuses_[address]);
				}

				if(!(g_game_field[address] == g_wall_symbol || IsBlockInsideGhostsRoom({x, y})))
				{
					continue;
				}

				if(MakeCollisionBetweenObjectAndBox(
					{
						IntToFixed16(x),
						IntToFixed16(y),
					},
					{
						IntToFixed16(x + 1),
						IntToFixed16(y + 1),
					},
					{ball_half_size, ball_half_size},
					pacman_.position,
					pacman_.arkanoid_ball->velocity))
				{
					// Rotate slightly velocity vector.
					const float random_angle = (float(rand_.Next()) / float(Rand::c_max_rand_plus_one_) - 0.5f) * 0.5f;
					const fixed16_t s = fixed16_t(std::sin(random_angle) * float(g_fixed16_one));
					const fixed16_t c = fixed16_t(std::cos(random_angle) * float(g_fixed16_one));

					pacman_.arkanoid_ball->velocity =
					{
						Fixed16Mul(pacman_.arkanoid_ball->velocity[0], c) - Fixed16Mul(pacman_.arkanoid_ball->velocity[1], s),
						Fixed16Mul(pacman_.arkanoid_ball->velocity[0], s) + Fixed16Mul(pacman_.arkanoid_ball->velocity[1], c),
					};

					sound_player_.PlaySound(SoundId::ArkanoidBallHit);
				}
			}
		}

		if(tick_ >= pacman_.arkanoid_ball->end_tick)
		{
			pacman_.arkanoid_ball = std::nullopt;
			pacman_.position[0] = IntToFixed16(Fixed16FloorToInt(pacman_.position[0])) + g_fixed16_one / 2;
			pacman_.position[1] = IntToFixed16(Fixed16FloorToInt(pacman_.position[1])) + g_fixed16_one / 2;
			pacman_.target_position = pacman_.position;
		}

		return;
	}

	fixed16vec2_t new_position = pacman_.position;
	switch(pacman_.direction)
	{
	case GridDirection::XMinus:
		new_position[0] -= g_pacman_move_speed;
		break;
	case GridDirection::XPlus:
		new_position[0] += g_pacman_move_speed;
		break;
	case GridDirection::YMinus:
		new_position[1] -= g_pacman_move_speed;
		break;
	case GridDirection::YPlus:
		new_position[1] += g_pacman_move_speed;
		break;
	}

	const fixed16vec2_t vec_to_target =
	{
		pacman_.target_position[0] - new_position[0],
		pacman_.target_position[1] - new_position[1],
	};
	if(Fixed16Abs(vec_to_target[0]) + Fixed16Abs(vec_to_target[1]) <= g_pacman_move_speed)
	{
		// Reached the target.

		const auto block_x = uint32_t(Fixed16FloorToInt(pacman_.target_position[0]));
		const auto block_y = uint32_t(Fixed16FloorToInt(pacman_.target_position[1]));
		PickUpBonus(bonuses_[block_x + block_y * c_field_width]);

		pacman_.position = pacman_.target_position;

		auto new_target_position = pacman_.target_position;
		switch(pacman_.next_direction)
		{
		case GridDirection::XMinus:
			new_target_position[0] -= g_fixed16_one;
			break;
		case GridDirection::XPlus:
			new_target_position[0] += g_fixed16_one;
			break;
		case GridDirection::YMinus:
			new_target_position[1] -= g_fixed16_one;
			break;
		case GridDirection::YPlus:
			new_target_position[1] += g_fixed16_one;
			break;
		}

		auto forward_target_position = pacman_.target_position;
		switch(pacman_.direction)
		{
		case GridDirection::XMinus:
			forward_target_position[0] -= g_fixed16_one;
			break;
		case GridDirection::XPlus:
			forward_target_position[0] += g_fixed16_one;
			break;
		case GridDirection::YMinus:
			forward_target_position[1] -= g_fixed16_one;
			break;
		case GridDirection::YPlus:
			forward_target_position[1] += g_fixed16_one;
			break;
		}

		const std::array<int32_t, 2> target_block =
		{
			Fixed16FloorToInt(new_target_position[0]), Fixed16FloorToInt(new_target_position[1])
		};
		const std::array<int32_t, 2> forward_block =
		{
			Fixed16FloorToInt(forward_target_position[0]), Fixed16FloorToInt(forward_target_position[1])
		};

		if( target_block[0] >= 0 && target_block[0] < int32_t(c_field_width ) &&
			target_block[1] >= 0 && target_block[1] < int32_t(c_field_height) &&
			g_game_field[uint32_t(target_block[0]) + uint32_t(target_block[1]) * c_field_width] != g_wall_symbol &&
			!IsBlockInsideGhostsRoom(target_block))
		{
			// Block towards target direction is free. Change target position and direction.
			pacman_.target_position = new_target_position;
			pacman_.direction = pacman_.next_direction;
		}
		else if(
			forward_block[0] >= 0 && forward_block[0] < int32_t(c_field_width ) &&
			forward_block[1] >= 0 && forward_block[1] < int32_t(c_field_height) &&
			g_game_field[uint32_t(forward_block[0]) + uint32_t(forward_block[1]) * c_field_width] != g_wall_symbol &&
			!IsBlockInsideGhostsRoom(forward_block))
		{
			// Forward block is free. Move towards it but do not change direction.
			pacman_.target_position = forward_target_position;
		}
	}
	else if(pacman_.direction == GridDirection::XPlus && pacman_.next_direction == GridDirection::XMinus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[0] -= g_fixed16_one;
	}
	else if(pacman_.direction == GridDirection::XMinus && pacman_.next_direction == GridDirection::XPlus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[0] += g_fixed16_one;
	}
	else if(pacman_.direction == GridDirection::YPlus && pacman_.next_direction == GridDirection::YMinus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[1] -= g_fixed16_one;
	}
	else if(pacman_.direction == GridDirection::YMinus && pacman_.next_direction == GridDirection::YPlus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[1] += g_fixed16_one;
	}
	else
	{
		pacman_.position = new_position;
	}
}

void GamePacman::MoveGhost(Ghost& ghost)
{
	if(tick_ < spawn_animation_end_tick_)
	{
		return;
	}

	uint32_t bonuses_release_limit = 0;
	switch(ghost.type)
	{
	case GhostType::Blinky:
		bonuses_release_limit = 0;
		break;
	case GhostType::Pinky:
		bonuses_release_limit = g_bonuses_eaten_for_pinky_release;
		break;
	case GhostType::Inky:
		bonuses_release_limit = g_bonuses_eaten_for_inky_release;
		break;
	case GhostType::Clyde:
		bonuses_release_limit = g_bonuses_eaten_for_clyde_release;
		break;
	}

	// Keep ghost inside home in early stages of level.
	// Use simple counter of bonuses for this.
	// This is not a proper way, but it works fine.
	if(bonuses_eaten_ < bonuses_release_limit)
	{
		return;
	}

	const fixed16_t speed = GetGhostSpeed(ghost.mode);

	fixed16vec2_t new_position = ghost.position;
	switch(ghost.direction)
	{
	case GridDirection::XMinus:
		new_position[0] -= speed;
		break;
	case GridDirection::XPlus:
		new_position[0] += speed;
		break;
	case GridDirection::YMinus:
		new_position[1] -= speed;
		break;
	case GridDirection::YPlus:
		new_position[1] += speed;
		break;
	}

	const fixed16vec2_t vec_to_target =
	{
		ghost.target_position[0] - new_position[0],
		ghost.target_position[1] - new_position[1],
	};
	if(Fixed16Abs(vec_to_target[0]) + Fixed16Abs(vec_to_target[1]) <= speed)
	{
		ghost.position = ghost.target_position;

		const std::array<int32_t, 2> block{
			Fixed16FloorToInt(ghost.target_position[0]),
			Fixed16FloorToInt(ghost.target_position[1])};

		if(ghost.mode == GhostMode::Eaten && IsBlockInsideGhostsRoom(block))
		{
			ghost.mode = current_ghosts_mode_;
		}

		if(ghost.mode == GhostMode::Frightened)
		{
			std::pair<std::array<int32_t, 2>, GridDirection> possible_targets[4];
			uint32_t num_possible_targets = 0;

			const auto add_next_block_candidate =
			[&](const std::array<int32_t, 2>& next_block, const GridDirection direction)
			{
				if(!IsBlockInsideGhostsRoom(block) && IsBlockInsideGhostsRoom(next_block))
				{
					// Do not allow to move into start room from outside.
					return;
				}
				if( next_block[0] >= 0 && next_block[0] < int32_t(c_field_width ) &&
					next_block[1] >= 0 && next_block[1] < int32_t(c_field_height))
				{
					const uint32_t address = uint32_t(next_block[0]) + uint32_t(next_block[1]) * c_field_width;
					if(g_game_field[address] != g_wall_symbol &&
						!(bonuses_[address] >= Bonus::TetrisBlock0 && bonuses_[address] <= Bonus::TetrisBlock6))
					{
						possible_targets[num_possible_targets] = std::make_pair(next_block, direction);
						++num_possible_targets;
					}
				}
			};

			// Never make 180 degrees turn.
			if(ghost.direction != GridDirection::YPlus )
			{
				add_next_block_candidate({block[0], block[1] - 1}, GridDirection::YMinus);
			}
			if(ghost.direction != GridDirection::XPlus )
			{
				add_next_block_candidate({block[0] - 1, block[1]}, GridDirection::XMinus);
			}
			if(ghost.direction != GridDirection::YMinus)
			{
				add_next_block_candidate({block[0], block[1] + 1}, GridDirection::YPlus );
			}
			if(ghost.direction != GridDirection::XMinus)
			{
				add_next_block_candidate({block[0] + 1, block[1]}, GridDirection::XPlus );
			}

			if(num_possible_targets != 0)
			{
				const auto selected_target = possible_targets[rand_.Next() % num_possible_targets];
				ghost.target_position = {
					IntToFixed16(selected_target.first[0]) + g_fixed16_one / 2,
					IntToFixed16(selected_target.first[1]) + g_fixed16_one / 2};
				ghost.direction = selected_target.second;
			}
			else
			{
				// Dead end.
				ReverseGhostMovement(ghost);
			}
		}
		else
		{
			const std::array<int32_t, 2> destination_block = GetGhostDestinationBlock(ghost.type, ghost.mode, block);

			int32_t best_square_distance  = 0x7FFFFFFF;
			std::array<int32_t, 2> best_next_block = block;
			GridDirection best_direction = ghost.direction;

			const auto add_next_block_candidate =
			[&](const std::array<int32_t, 2>& next_block, const GridDirection direction)
			{
				if(ghost.mode != GhostMode::Eaten &&
					!IsBlockInsideGhostsRoom(block) && IsBlockInsideGhostsRoom(next_block))
				{
					// Do not allow to move into start room from outside.
					return;
				}

				const std::array<int32_t, 2> vec_to
					{next_block[0] - destination_block[0], next_block[1] - destination_block[1]};
				const int32_t square_distance = vec_to[0] * vec_to[0] + vec_to[1] * vec_to[1];
				if(square_distance < best_square_distance &&
					next_block[0] >= 0 && next_block[0] < int32_t(c_field_width ) &&
					next_block[1] >= 0 && next_block[1] < int32_t(c_field_height))
				{
					const uint32_t address = uint32_t(next_block[0]) + uint32_t(next_block[1]) * c_field_width;
					if(g_game_field[address] != g_wall_symbol &&
						!(bonuses_[address] >= Bonus::TetrisBlock0 && bonuses_[address] <= Bonus::TetrisBlock6))
					{
						best_square_distance = square_distance;
						best_next_block = next_block;
						best_direction = direction;
					}
				}
			};

			// Order conditions by priority as in original game, see https://youtu.be/ataGotQ7ir8?t=180.
			// Never make 180 degrees turn.
			if(ghost.direction != GridDirection::YPlus )
			{
				add_next_block_candidate({block[0], block[1] - 1}, GridDirection::YMinus);
			}
			if(ghost.direction != GridDirection::XPlus )
			{
				add_next_block_candidate({block[0] - 1, block[1]}, GridDirection::XMinus);
			}
			if(ghost.direction != GridDirection::YMinus)
			{
				add_next_block_candidate({block[0], block[1] + 1}, GridDirection::YPlus );
			}
			if(ghost.direction != GridDirection::XMinus)
			{
				add_next_block_candidate({block[0] + 1, block[1]}, GridDirection::XPlus );
			}

			if(best_square_distance == 0x7FFFFFFF)
			{
				// Dead end.
				ReverseGhostMovement(ghost);
			}
			else
			{
				ghost.target_position = {
					IntToFixed16(best_next_block[0]) + g_fixed16_one / 2,
					IntToFixed16(best_next_block[1]) + g_fixed16_one / 2};
				ghost.direction = best_direction;
			}
		}
	}
	else
	{
		ghost.position = new_position;
	}

	if(ghost.mode == GhostMode::Frightened && tick_ >= ghost.frightened_mode_end_tick)
	{
		ghost.mode = current_ghosts_mode_;
	}
}

std::array<int32_t, 2> GamePacman::GetGhostDestinationBlock(
	const GhostType ghost_type,
	const GhostMode ghost_mode,
	const std::array<int32_t, 2>& ghost_position)
{
	if(ghost_mode == GhostMode::Eaten)
	{
		// Move towards ghosts room while eaten.
		return {16, 15};
	}

	// If ghost is in the middle room - target towards exit from this room.
	if(IsBlockInsideGhostsRoom(ghost_position))
	{
		return {20, 15};
	}

	if(ghost_mode == GhostMode::Scatter)
	{
		return GetScatterModeTarget(ghost_type);
	}

	const std::array<int32_t, 2> pacman_block{
		Fixed16FloorToInt(pacman_.target_position[0]),
		Fixed16FloorToInt(pacman_.target_position[1])};

	switch(ghost_type)
	{
	case GhostType::Blinky:
		return pacman_block;

	case GhostType::Pinky:
		{
			auto block = pacman_block;
			const int32_t offset = 4;
			// TODO - maybe simulate original diagonal offset bug?
			switch(pacman_.direction)
			{
			case GridDirection::XMinus:
				block[0] -= offset;
				break;
			case GridDirection::XPlus :
				block[0] += offset;
				break;
			case GridDirection::YMinus:
				block[1] -= offset;
				break;
			case GridDirection::YPlus :
				block[1] += offset;
				break;
			}
			return block;
		}

	case GhostType::Inky:
		for(const Ghost& other_ghost : ghosts_)
		{
			if(other_ghost.type != GhostType::Blinky)
			{
				continue;
			}

			auto block = pacman_block;
			const int32_t offset = 2;
			// TODO - maybe simulate original diagonal offset bug?
			switch(pacman_.direction)
			{
			case GridDirection::XMinus:
				block[0] -= offset;
				break;
			case GridDirection::XPlus :
				block[0] += offset;
				break;
			case GridDirection::YMinus:
				block[1] -= offset;
				break;
			case GridDirection::YPlus :
				block[1] += offset;
				break;
			}

			const std::array<int32_t, 2> vec_to_blinky
			{
				Fixed16FloorToInt(other_ghost.position[0]) - block[0],
				Fixed16FloorToInt(other_ghost.position[1]) - block[1],
			};

			return
			{
				block[0] - vec_to_blinky[0],
				block[1] - vec_to_blinky[1],
			};
		}
		// Can't find Blinky.
		return pacman_block;

	case GhostType::Clyde:
		{
			const std::array<int32_t, 2> vec_to_pacman
			{
				pacman_block[0] - ghost_position[0],
				pacman_block[1] - ghost_position[1],
			};
			const int32_t square_dist = vec_to_pacman[0] * vec_to_pacman[0] + vec_to_pacman[1] * vec_to_pacman[1];
			const int32_t threshold_dist = 8;
			if(square_dist >= threshold_dist * threshold_dist)
			{
				return pacman_block;
			}

			// TODO - move scatter mode constants to another place.
			return GetScatterModeTarget(ghost_type);
		}
	}

	assert(false);
	return pacman_block;
}

void GamePacman::ProcessPacmanGhostsTouch()
{
	const fixed16_t touch_dist = g_fixed16_one / 3;
	const fixed16_t touch_square_dist = Fixed16Mul(touch_dist, touch_dist);

	for(Ghost& ghost : ghosts_)
	{
		if(ghost.mode == GhostMode::Eaten)
		{
			continue;
		}

		const fixed16vec2_t vec_from_ghost_to_pacman
		{
			ghost.position[0] - pacman_.position[0],
			ghost.position[1] - pacman_.position[1],
		};
		const fixed16_t square_dist = Fixed16VecSquareLen(vec_from_ghost_to_pacman);
		if(square_dist < touch_square_dist)
		{
			if(ghost.mode == GhostMode::Frightened || pacman_.arkanoid_ball != std::nullopt)
			{
				ghost.mode = GhostMode::Eaten;
				score_ += g_score_for_ghost;
				sound_player_.PlaySound(SoundId::ArkanoidBallHit);
			}
			else
			{
				if(pacman_.dead_animation_end_tick == std::nullopt)
				{
					pacman_.dead_animation_end_tick = tick_ + g_death_animation_duration;
					sound_player_.PlaySound(SoundId::CharacterDeath);
				}
			}
		}
	}
}

void GamePacman::TryTeleportCharacters()
{
	const uint32_t teleport_x = 17;
	const uint32_t teleport_y_0 = 0;
	const uint32_t teleport_y_1 = c_field_height - 1;

	const fixed16_t teleport_distance = IntToFixed16(int32_t(teleport_y_1 - teleport_y_0 - 1));

	const std::array<uint32_t, 2> pacman_block{
		uint32_t(Fixed16FloorToInt(pacman_.position[0])),
		uint32_t(Fixed16FloorToInt(pacman_.position[1]))};

	if(pacman_block[0] == teleport_x)
	{
		if(pacman_block[1] == teleport_y_0)
		{
			pacman_.position[1] += teleport_distance;
			pacman_.target_position[1] += teleport_distance;
		}
		if(pacman_block[1] == teleport_y_1)
		{
			pacman_.position[1] -= teleport_distance;
			pacman_.target_position[1] -= teleport_distance;
		}
	}

	for(Ghost& ghost : ghosts_)
	{
		const std::array<uint32_t, 2> ghost_block{
			uint32_t(Fixed16FloorToInt(ghost.position[0])),
			uint32_t(Fixed16FloorToInt(ghost.position[1]))};
		if(ghost_block[1] == teleport_y_0)
		{
			ghost.position[1] += teleport_distance;
			ghost.target_position[1] += teleport_distance;
		}
		if(ghost_block[1] == teleport_y_1)
		{
			ghost.position[1] -= teleport_distance;
			ghost.target_position[1] -= teleport_distance;
		}
	}
}

bool GamePacman::UpdateLaserBeam(LaserBeam& laser_beam)
{
	switch(laser_beam.direction)
	{
	case GridDirection::XMinus:
		laser_beam.position[0] -= g_laser_beam_speed;
		break;
	case GridDirection::XPlus:
		laser_beam.position[0] += g_laser_beam_speed;
		break;
	case GridDirection::YMinus:
		laser_beam.position[1] -= g_laser_beam_speed;
		break;
	case GridDirection::YPlus:
		laser_beam.position[1] += g_laser_beam_speed;
		break;
	}

	const fixed16_t beam_half_size = g_fixed16_one / 32;

	for(Ghost& ghost : ghosts_)
	{
		if(ghost.mode == GhostMode::Eaten)
		{
			continue;
		}

		const fixed16vec2_t borders_min =
		{
			ghost.position[0] - (beam_half_size + g_character_half_size),
			ghost.position[1] - (beam_half_size + g_character_half_size),
		};
		const fixed16vec2_t borders_max =
		{
			ghost.position[0] + (beam_half_size + g_character_half_size),
			ghost.position[1] + (beam_half_size + g_character_half_size),
		};

		if( laser_beam.position[0] >= borders_min[0] && laser_beam.position[0] <= borders_max[0] &&
			laser_beam.position[1] >= borders_min[1] && laser_beam.position[1] <= borders_max[1])
		{
			ghost.mode = GhostMode::Eaten;
			score_ += g_score_for_ghost;
			sound_player_.PlaySound(SoundId::ArkanoidBallHit);
			// Destroy laser beam at first hit.
			return true;
		}
	}

	const int32_t x_center = Fixed16RoundToInt(laser_beam.position[0]);
	const int32_t y_center = Fixed16RoundToInt(laser_beam.position[1]);

	for(int32_t dy = -3; dy <= 3; ++dy)
	{
		const int32_t y = y_center + dy;
		if(y < 0 || y >= int32_t(c_field_height))
		{
			continue;
		}

		for(int32_t dx = -3; dx <= 3; ++dx)
		{
			const int32_t x = x_center + dx;
			if(x < 0 || x >= int32_t(c_field_width))
			{
				continue;
			}

			const uint32_t address = uint32_t(x) + uint32_t(y) * c_field_width;

			if(!(g_game_field[address] == g_wall_symbol || IsBlockInsideGhostsRoom({x, y})))
			{
				continue;
			}

			const fixed16vec2_t borders_min =
			{
				IntToFixed16(x) - beam_half_size,
				IntToFixed16(y) - beam_half_size,
			};
			const fixed16vec2_t borders_max =
			{
				IntToFixed16(x + 1) + beam_half_size,
				IntToFixed16(y + 1) + beam_half_size,
			};

			if( laser_beam.position[0] >= borders_min[0] && laser_beam.position[0] <= borders_max[0] &&
				laser_beam.position[1] >= borders_min[1] && laser_beam.position[1] <= borders_max[1])
			{
				sound_player_.PlaySound(SoundId::ArkanoidBallHit);
				// Destroy laser beam at first hit.
				return true;
			}
		}
	}

	return
		laser_beam.position[0] <= g_fixed16_one || laser_beam.position[0] >= IntToFixed16(int32_t(c_field_width  - 1)) ||
		laser_beam.position[1] <= g_fixed16_one || laser_beam.position[1] >= IntToFixed16(int32_t(c_field_height - 1));
}

void GamePacman::PickUpBonus(Bonus& bonus)
{
	if(bonus == Bonus::None)
	{
		return;
	}

	if(bonus == Bonus::Food || (bonus >= Bonus::TetrisBlock0 && bonus <= Bonus::TetrisBlock6))
	{
		score_ += g_score_for_food;
		sound_player_.PlaySound(SoundId::TetrisFigureStep);
	}
	if(bonus == Bonus::Deadly)
	{
		score_ += g_score_for_deadly_bonus;
		EnterFrightenedMode();
		sound_player_.PlaySound(SoundId::SnakeBonusEat);
	}
	if(bonus == Bonus::SnakeFoodSmall)
	{
		for(Ghost& ghost : ghosts_)
		{
			ReverseGhostMovement(ghost);
		}
		score_ += g_score_for_snake_bonus;
		sound_player_.PlaySound(SoundId::SnakeBonusEat);
	}
	if(bonus == Bonus::SnakeFoodMedium)
	{
		pacman_.turret_shots_left = 3;
		score_ += g_score_for_snake_bonus;
		sound_player_.PlaySound(SoundId::TetrisFigureStep);
	}
	if(bonus == Bonus::SnakeFoodLarge)
	{
		ArkanoidBallModifier arkanod_ball;
		arkanod_ball.end_tick = tick_ + g_arkanoid_ball_mode_duration;

		const float random_angle = rand_.RandomAngle();
		const fixed16_t speed = g_fixed16_one / 10;
		arkanod_ball.velocity[0] = fixed16_t(std::cos(random_angle) * float(speed));
		arkanod_ball.velocity[1] = fixed16_t(std::sin(random_angle) * float(speed));

		pacman_.arkanoid_ball = arkanod_ball;
		score_ += g_score_for_snake_bonus;
		sound_player_.PlaySound(SoundId::TetrisFigureStep);
	}
	if(bonus == Bonus::SnakeExtraLife)
	{
		lifes_ = std::min(lifes_ + 1, g_max_lifes);
		sound_player_.PlaySound(SoundId::TetrisFigureStep);
	}

	bonus = Bonus::None;
	--bonuses_left_;
	++bonuses_eaten_;

	TryPlaceRandomTetrisPiece();
	TrySpawnSnakeBonus();
}

void GamePacman::UpdateGhostsMode()
{
	// See https://www.gamedeveloper.com/design/the-pac-man-dossier.

	if(ghosts_mode_switches_left_ == 0)
	{
		assert(current_ghosts_mode_ == GhostMode::Chase);
		return;
	}

	bool some_is_frightened = false;
	for(const Ghost& ghost : ghosts_)
	{
		some_is_frightened |= ghost.mode == GhostMode::Frightened;
	}

	if(some_is_frightened)
	{
		// Pause mode switches while ghosts are frightened.
		++next_ghosts_mode_swith_tick_;
	}

	if(tick_ < next_ghosts_mode_swith_tick_)
	{
		return;
	}

	if(current_ghosts_mode_ == GhostMode::Scatter)
	{
		current_ghosts_mode_ = GhostMode::Chase;
		--ghosts_mode_switches_left_;
		next_ghosts_mode_swith_tick_ = tick_ + g_chase_duration;
	}
	else
	{
		current_ghosts_mode_ = GhostMode::Scatter;
		const uint32_t duration =
			ghosts_mode_switches_left_ >= 3 ? g_scatter_duration_first : g_scatter_duration_second;
		next_ghosts_mode_swith_tick_ = tick_ + duration;
	}

	for(Ghost& ghost : ghosts_)
	{
		if(ghost.mode != GhostMode::Frightened && ghost.mode != GhostMode::Eaten)
		{
			ghost.mode = current_ghosts_mode_;
			ReverseGhostMovement(ghost);
		}
	}
}

void GamePacman::EnterFrightenedMode()
{
	for(Ghost& ghost : ghosts_)
	{
		ghost.mode = GhostMode::Frightened;
		ghost.frightened_mode_end_tick = tick_ + g_frightened_mode_duration;

		ReverseGhostMovement(ghost);
	}
}

void GamePacman::TryPlaceRandomTetrisPiece()
{
	if(tick_ < g_transition_time_change_end)
	{
		return;
	}

	if(rand_.Next() % 48 != 17)
	{
		return;
	}

	// Perform spawn possibility check for multiple random positions across game field.
	for(size_t i = 0; i < 256; ++i)
	{
		uint32_t type_index = rand_.Next() % g_tetris_num_piece_types;
		if(type_index == 0)
		{
			// Reduce probability of "I" piece, because it is too boring.
			type_index = rand_.Next() % g_tetris_num_piece_types;
		}
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

		int32_t min_x = 99999, max_x = -9999;
		int32_t min_y = 99999, max_y = -9999;
		for(const TetrisPieceBlock& block : blocks)
		{
			min_x = std::min(min_x, block[0]);
			max_x = std::max(max_x, block[0]);
			min_y = std::min(min_y, block[1]);
			max_y = std::max(max_y, block[1]);
		}

		const int32_t min_dx = 1 - min_x;
		const int32_t max_dx = int32_t(c_field_width ) - max_x - 2;

		const int32_t min_dy = 1 - min_y;
		const int32_t max_dy = int32_t(c_field_height) - max_y - 2;

		const int32_t dx = min_dx + int32_t(rand_.Next() % uint32_t(max_dx - min_dx));
		const int32_t dy = min_dy + int32_t(rand_.Next() % uint32_t(max_dy - min_dy));

		TetrisPieceBlocks bocks_shifted = blocks;
		for(TetrisPieceBlock& block : bocks_shifted)
		{
			const int32_t x = block[0] + dx;
			const int32_t y = block[1] + dy;
			assert(x >= 1 && x < int32_t(c_field_width ) - 1);
			assert(y >= 1 && y < int32_t(c_field_height) - 1);

			block[0] = x;
			block[1] = y;
		}

		bool can_place = true;
		for(const TetrisPieceBlock& block : bocks_shifted)
		{
			const uint32_t address = uint32_t(block[0]) + uint32_t(block[1]) * c_field_width;
			can_place &= g_game_field[address] != g_wall_symbol;
			can_place &= bonuses_[address] == Bonus::None;
			can_place &= !IsBlockInsideGhostsRoom(block);

			// Ghost room entrance.
			can_place &=
				!(block[0] == 20 && block[1] >= 11 && block[1] <= 19);

			// Unreachable areas.
			if(block[0] >= 13 && block[0] <= 15)
			{
				can_place &= !(block[1] >= 0 && block[1] <= 5);
				can_place &= !(block[1] >= 24 && block[1] <= 29);
			}
			if(block[0] >= 19 && block[0] <= 21)
			{
				can_place &= !(block[1] >= 0 && block[1] <= 5);
				can_place &= !(block[1] >= 24 && block[1] <= 29);
			}

			// Do not place block atop of characters.
			if(pacman_.position[0] >= IntToFixed16(block[0]) - g_character_half_size &&
			   pacman_.position[0] <= IntToFixed16(block[0] + 1) + g_character_half_size &&
			   pacman_.position[1] >= IntToFixed16(block[1]) - g_character_half_size &&
			   pacman_.position[1] <= IntToFixed16(block[1] + 1) + g_character_half_size)
			{
				can_place = false;
			}
			for(const Ghost& ghost : ghosts_)
			{
				if(ghost.position[0] >= IntToFixed16(block[0]) - g_character_half_size &&
				   ghost.position[0] <= IntToFixed16(block[0] + 1) + g_character_half_size &&
				   ghost.position[1] >= IntToFixed16(block[1]) - g_character_half_size &&
				   ghost.position[1] <= IntToFixed16(block[1] + 1) + g_character_half_size)
				{
					can_place = false;
				}
			}
		}

		if(can_place)
		{
			for(const TetrisPieceBlock& block : bocks_shifted)
			{
				bonuses_[uint32_t(block[0]) + uint32_t(block[1]) * c_field_width] =
					Bonus(uint32_t(Bonus::TetrisBlock0) + type_index);
				++bonuses_left_;
			}
			break;
		}
	}
}

void GamePacman::TrySpawnSnakeBonus()
{
	if(tick_ < g_transition_time_change_end)
	{
		return;
	}

	if(rand_.Next() % 24 != 3)
	{
		return;
	}

	// Perform spawn possibility check for multiple random positions across game field.
	for(size_t i = 0; i < 256; ++i)
	{
		const uint32_t x = 1 + rand_.Next() % (c_field_width  - 2);
		const uint32_t y = 1 + rand_.Next() % (c_field_height - 2);

		bool can_place = true;

		const uint32_t address = x + y * c_field_width;
		can_place &= g_game_field[address] != g_wall_symbol;
		can_place &= bonuses_[address] == Bonus::None;
		can_place &= !IsBlockInsideGhostsRoom({int32_t(x), int32_t(y)});

		// Unreachable areas.
		if(x >= 13 && x <= 15)
		{
			can_place &= !(y >= 0 && y <= 5);
			can_place &= !(y >= 24 && y <= 29);
		}
		if(x >= 19 && x <= 21)
		{
			can_place &= !(y >= 0 && y <= 5);
			can_place &= !(y >= 24 && y <= 29);
		}

		if(can_place)
		{
			const uint32_t r = rand_.Next() % 11;
			if(r < 4)
			{
				bonuses_[address] = Bonus::SnakeFoodSmall;
			}
			else if(r < 7)
			{
				bonuses_[address] = Bonus::SnakeFoodMedium;
			}
			else if(r < 9)
			{
				bonuses_[address] = Bonus::SnakeFoodLarge;
			}
			else
			{
				bonuses_[address] = Bonus::SnakeExtraLife;
			}

			++bonuses_left_;
			break;
		}
	}
}

void GamePacman::UpdateSnakePosition()
{
	if(tick_ == g_transition_time_change_snake)
	{
		pacman_.target_position = temp_snake_position_;
		pacman_.position = pacman_.target_position;
	}
	if(tick_ >= g_transition_time_change_snake)
	{
		return;
	}

	const uint32_t diff = (tick_ + 1) / g_transition_snake_move_speed - tick_ / g_transition_snake_move_speed;
	for(uint32_t i = 0; i < diff; ++i)
	{
		temp_snake_position_[1] += IntToFixed16(10) / int32_t(c_block_size);

		if(!snake_transition_bonuses_.empty() && snake_transition_bonuses_.front().position[1] <= temp_snake_position_[1])
		{
			snake_transition_bonuses_.erase(snake_transition_bonuses_.begin());
			sound_player_.PlaySound(SoundId::SnakeBonusEat);
		}
		else
		{
			sound_player_.PlaySound(SoundId::TetrisFigureStep);
		}
	}
}

bool GamePacman::IsBlockInsideGhostsRoom(const std::array<int32_t, 2>& block)
{
	return
		block[0] >= 16 && block[0] <= 19 &&
		block[1] >= 12 && block[1] <= 17;
}

std::array<int32_t, 2> GamePacman::GetScatterModeTarget(const GhostType ghost_type)
{
	switch(ghost_type)
	{
	case GhostType::Blinky: return {int32_t(c_field_width) + 2, int32_t(c_field_height - 4)};
	case GhostType::Pinky: return {int32_t(c_field_width) + 2, 3};
	case GhostType::Inky: return {0, int32_t(c_field_height - 1)};
	case GhostType::Clyde: return {0, 1};
	}

	assert(false);
	return {0, 0};
}

void GamePacman::ReverseGhostMovement(Ghost& ghost)
{
	switch(ghost.direction)
	{
	case GridDirection::XPlus:
		ghost.target_position[0] -= g_fixed16_one;
		ghost.direction = GridDirection::XMinus;
		break;
	case GridDirection::XMinus:
		ghost.target_position[0] += g_fixed16_one;
		ghost.direction = GridDirection::XPlus;
		break;
	case GridDirection::YPlus :
		ghost.target_position[1] -= g_fixed16_one;
		ghost.direction = GridDirection::YMinus;
		break;
	case GridDirection::YMinus:
		ghost.target_position[1] += g_fixed16_one;
		ghost.direction = GridDirection::YPlus;
		break;
	}
}

fixed16_t GamePacman::GetGhostSpeed(const GhostMode ghost_mode)
{
	switch(ghost_mode)
	{
	case GhostMode::Chase:
	case GhostMode::Scatter:
		return g_ghost_move_speed;
	case GhostMode::Frightened:
		return g_ghost_frightened_move_speed;
	case GhostMode::Eaten:
		return g_ghost_eaten_move_speed;
	}

	assert(false);
	return g_ghost_move_speed;
}
