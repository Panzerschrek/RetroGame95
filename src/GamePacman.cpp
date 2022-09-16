#include "GamePacman.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "Sprites.hpp"
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

const uint32_t g_frightened_mode_duration = GameInterface::c_update_frequency * 10;
const uint32_t g_death_animation_duration = GameInterface::c_update_frequency * 3 / 2;
const uint32_t g_spawn_animation_duration = GameInterface::c_update_frequency * 3;

const uint32_t g_scatter_duration_first = 7 * GameInterface::c_update_frequency * 3 / 2;
const uint32_t g_scatter_duration_second = 5 * GameInterface::c_update_frequency * 3 / 2;
const uint32_t g_chase_duration = 20 * GameInterface::c_update_frequency * 3 / 2;

const uint32_t g_bonuses_eaten_for_pinky_release = 0;
const uint32_t g_bonuses_eaten_for_inky_release = g_bonuses_eaten_for_pinky_release + 30;
const uint32_t g_bonuses_eaten_for_clyde_release = g_bonuses_eaten_for_inky_release + 45;

const uint32_t g_max_lifes = 8;

} // namespace

GamePacman::GamePacman(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
	SpawnPacmanAndGhosts();
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
		}
	}

	++tick_;

	UpdateGhostsMode();

	MovePacman();

	for(Ghost& ghost : ghosts_)
	{
		MoveGhost(ghost);
	}

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

	for(uint32_t y = 0; y < c_field_height; ++y)
	for(uint32_t x = 0; x < c_field_width ; ++x)
	{
		const Bonus bonus = bonuses_[x + y * c_field_width];
		const uint32_t block_x = x * c_block_size;
		const uint32_t block_y = y * c_block_size;

		switch(bonus)
		{
		case Bonus::None:
			break;
		case Bonus::Food:
			DrawSprite(frame_buffer, Sprites::pacman_food, block_x + 3, block_y + 3);
			break;
		case Bonus::Deadly:
			DrawSprite(frame_buffer, Sprites::pacman_bonus_deadly, block_x, block_y);
			break;
		}
	}

	for(const Ghost& ghost : ghosts_)
	{
		if(ghost.mode == GhostMode::Frightened || ghost.mode == GhostMode::Eaten)
		{
			DrawGhost(frame_buffer, ghost);
		}
	}

	DrawPacman(frame_buffer);

	for(const Ghost& ghost : ghosts_)
	{
		if(!(ghost.mode == GhostMode::Frightened || ghost.mode == GhostMode::Eaten))
		{
			DrawGhost(frame_buffer, ghost);
		}
	}

	const SpriteBMP life_spirte(Sprites::pacman_life);
	for(uint32_t i = 0; i < lifes_; ++i)
	{
		DrawSpriteWithAlpha(
			frame_buffer,
			life_spirte,
			0,
			c_field_width * c_block_size + i % 4 * (life_spirte.GetWidth() + 1),
			c_block_size + i / 4 * (life_spirte.GetHeight() + 2));
	}

	if(tick_ < spawn_animation_end_tick_)
	{
		DrawTextCentered(
			frame_buffer,
			g_color_white,
			c_field_width  * c_block_size / 2,
			c_field_height * c_block_size / 2,
			"Ready!");
	}
	if(game_over_)
	{
		DrawTextCentered(
			frame_buffer,
			g_color_white,
			c_field_width  * c_block_size / 2,
			c_field_height * c_block_size / 2,
			"game over");
	}
}

GameInterfacePtr GamePacman::AskForNextGameTransition()
{
	return std::move(next_game_);
}

void GamePacman::DrawPacman(const FrameBuffer frame_buffer) const
{
	if(pacman_.dead_animation_end_tick != std::nullopt)
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

		const SpriteBMP current_sprite =
			tick_ < spawn_animation_end_tick_ ? sprites[1] : sprites[tick_ / 12 % std::size(sprites)];
		const uint32_t pacman_x =
			uint32_t(Fixed16FloorToInt(pacman_.position[0] * int32_t(c_block_size))) - current_sprite.GetWidth() / 2;
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

	char text[64];
	std::snprintf(text, sizeof(text), "level %1d", level_);
	DrawText(frame_buffer, g_color_white, 260, 40, text);
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

	bonuses_eaten_ = 0;

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

void GamePacman::MovePacman()
{
	if(pacman_.dead_animation_end_tick != std::nullopt || tick_ < spawn_animation_end_tick_)
	{
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
		Bonus& bonus = bonuses_[block_x + block_y * c_field_width];
		if(bonus != Bonus::None)
		{
			if(bonus == Bonus::Deadly)
			{
				EnterFrightenedMode();
			}
			bonus = Bonus::None;
			--bonuses_left_;
			++bonuses_eaten_;
			// TODO - add score here.
		}

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
				if(
					next_block[0] >= 0 && next_block[0] < int32_t(c_field_width ) &&
					next_block[1] >= 0 && next_block[1] < int32_t(c_field_height) &&
					g_game_field[uint32_t(next_block[0]) + uint32_t(next_block[1]) * c_field_width] != g_wall_symbol)
				{
					possible_targets[num_possible_targets] = std::make_pair(next_block, direction);
					++num_possible_targets;
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
					next_block[1] >= 0 && next_block[1] < int32_t(c_field_height) &&
					g_game_field[uint32_t(next_block[0]) + uint32_t(next_block[1]) * c_field_width] != g_wall_symbol)
				{
					best_square_distance = square_distance;
					best_next_block = next_block;
					best_direction = direction;
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

			ghost.target_position = {
				IntToFixed16(best_next_block[0]) + g_fixed16_one / 2,
				IntToFixed16(best_next_block[1]) + g_fixed16_one / 2};
			ghost.direction = best_direction;
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
			if(ghost.mode == GhostMode::Frightened)
			{
				ghost.mode = GhostMode::Eaten;
			}
			else
			{
				if(pacman_.dead_animation_end_tick == std::nullopt)
				{
					pacman_.dead_animation_end_tick = tick_ + g_death_animation_duration;
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
