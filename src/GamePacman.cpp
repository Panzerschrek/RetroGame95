#include "GamePacman.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "Sprites.hpp"

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

const fixed16_t g_pacman_move_speed = g_fixed16_one / 32;

} // namespace

GamePacman::GamePacman(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
	pacman_.target_position = {IntToFixed16(8) + g_fixed16_one / 2, IntToFixed16(12) + g_fixed16_one / 2};
	pacman_.direction = PacmanDirection::YPlus;
	pacman_.next_direction = pacman_.direction;
	pacman_.position = pacman_.target_position;

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
			break;
		case g_bonus_deadly_symbol:
			bonus = Bonus::Deadly;
			break;
		default:
			break;
		}
	}
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
		if(event.type == SDL_KEYDOWN)
		{
			if(event.key.keysym.scancode == SDL_SCANCODE_LEFT)
			{
				pacman_.next_direction = PacmanDirection::XMinus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
			{
				pacman_.next_direction = PacmanDirection::XPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_DOWN)
			{
				pacman_.next_direction = PacmanDirection::YPlus;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_UP)
			{
				pacman_.next_direction = PacmanDirection::YMinus;
			}
		}
	}

	++tick_;

	fixed16vec2_t new_position = pacman_.position;
	switch(pacman_.direction)
	{
	case PacmanDirection::XMinus:
		new_position[0] -= g_pacman_move_speed;
		break;
	case PacmanDirection::XPlus:
		new_position[0] += g_pacman_move_speed;
		break;
	case PacmanDirection::YMinus:
		new_position[1] -= g_pacman_move_speed;
		break;
	case PacmanDirection::YPlus:
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
			bonus = Bonus::None;
			// TODO - add score here.
		}

		pacman_.position = pacman_.target_position;

		auto new_target_position = pacman_.target_position;
		switch(pacman_.next_direction)
		{
		case PacmanDirection::XMinus:
			new_target_position[0] -= g_fixed16_one;
			break;
		case PacmanDirection::XPlus:
			new_target_position[0] += g_fixed16_one;
			break;
		case PacmanDirection::YMinus:
			new_target_position[1] -= g_fixed16_one;
			break;
		case PacmanDirection::YPlus:
			new_target_position[1] += g_fixed16_one;
			break;
		}

		auto forward_target_position = pacman_.target_position;
		switch(pacman_.direction)
		{
		case PacmanDirection::XMinus:
			forward_target_position[0] -= g_fixed16_one;
			break;
		case PacmanDirection::XPlus:
			forward_target_position[0] += g_fixed16_one;
			break;
		case PacmanDirection::YMinus:
			forward_target_position[1] -= g_fixed16_one;
			break;
		case PacmanDirection::YPlus:
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
			g_game_field[uint32_t(target_block[0]) + uint32_t(target_block[1]) * c_field_width] != g_wall_symbol)
		{
			// Block towards target direction is free. Change target position and direction.
			pacman_.target_position = new_target_position;
			pacman_.direction = pacman_.next_direction;
		}
		else if(
			forward_block[0] >= 0 && forward_block[0] < int32_t(c_field_width ) &&
			forward_block[1] >= 0 && forward_block[1] < int32_t(c_field_height) &&
			g_game_field[uint32_t(forward_block[0]) + uint32_t(forward_block[1]) * c_field_width] != g_wall_symbol)
		{
			// Forward block is free. Move towards it but do not change direction.
			pacman_.target_position = forward_target_position;
		}
	}
	else if(pacman_.direction == PacmanDirection::XPlus && pacman_.next_direction == PacmanDirection::XMinus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[0] -= g_fixed16_one;
	}
	else if(pacman_.direction == PacmanDirection::XMinus && pacman_.next_direction == PacmanDirection::XPlus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[0] += g_fixed16_one;
	}
	else if(pacman_.direction == PacmanDirection::YPlus && pacman_.next_direction == PacmanDirection::YMinus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[1] -= g_fixed16_one;
	}
	else if(pacman_.direction == PacmanDirection::YMinus && pacman_.next_direction == PacmanDirection::YPlus)
	{
		pacman_.direction = pacman_.next_direction;
		pacman_.target_position[1] += g_fixed16_one;
	}
	else
	{
		pacman_.position = new_position;
	}
}

void GamePacman::Draw(const FrameBuffer frame_buffer) const
{
	static_assert(std::size(g_game_field) == c_field_width * c_field_height + 1, "Wrong field size");

	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const Color32 c_wall_color = 0x000000FF;
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

	const SpriteBMP pacman_sprites[]
	{
		Sprites::pacman_0,
		Sprites::pacman_1,
		Sprites::pacman_2,
		Sprites::pacman_3,
		Sprites::pacman_2,
		Sprites::pacman_1,
	};

	const SpriteBMP current_sprite = pacman_sprites[tick_ / 12 % std::size(pacman_sprites)];
	const uint32_t pacman_x =
		uint32_t(Fixed16FloorToInt(pacman_.position[0] * int32_t(c_block_size))) - current_sprite.GetWidth() / 2;
	const uint32_t pacman_y =
		uint32_t(Fixed16FloorToInt(pacman_.position[1] * int32_t(c_block_size))) - current_sprite.GetHeight() / 2;
	switch(pacman_.direction)
	{
	case PacmanDirection::XMinus:
		DrawSpriteWithAlphaRotate180(frame_buffer, current_sprite, 0, pacman_x, pacman_y);
		break;
	case PacmanDirection::XPlus:
		DrawSpriteWithAlpha         (frame_buffer, current_sprite, 0, pacman_x, pacman_y);
		break;
	case PacmanDirection::YMinus:
		DrawSpriteWithAlphaRotate270(frame_buffer, current_sprite, 0, pacman_x, pacman_y);
		break;
	case PacmanDirection::YPlus:
		DrawSpriteWithAlphaRotate90 (frame_buffer, current_sprite, 0, pacman_x, pacman_y);
		break;
	}
}

GameInterfacePtr GamePacman::AskForNextGameTransition()
{
	return std::move(next_game_);
}

