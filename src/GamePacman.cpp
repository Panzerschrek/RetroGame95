#include "GamePacman.hpp"
#include "Draw.hpp"

namespace
{

constexpr char g_game_field[]=
"#################################"
"#                               #"
"#               ######          #"
"#               ######          #"
"#               ##  ##          #"
"#               ##  ##          #"
"#               ######          #"
"#               ######          #"
"#                               #"
"#                               #"
"#                               #"
"#          #         ####       #"
"#          #         ####       #"
"#          #         ####       #"
"#                               #"
"#                               #"
"#                               #"
"#             ####              #"
"#             ####              #"
"#                               #"
"#                    ##         #"
"#                    ##         #"
"#                    ##         #"
"#                    ##  ##     #"
"#                        ##     #"
"#          ###         ######   #"
"#                      ######   #"
"#                        ##     #"
"#                               #"
"#################################"

;

} // namespace

GamePacman::GamePacman(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
}

void GamePacman::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void)events;
	(void)keyboard_state;
}

void GamePacman::Draw(const FrameBuffer frame_buffer) const
{
	static_assert(std::size(g_game_field) == c_field_width * c_field_height + 1, "Wrong field size");

	FillWholeFrameBuffer(frame_buffer, g_color_black);

	const Color32 c_wall_color = 0x0000007F;
	for(uint32_t y = 0; y < c_field_height; ++y)
	{
		const char* const line = g_game_field + y * c_field_width;
		const char* const line_y_minus = g_game_field + (std::max(1u, y) - 1) * c_field_width;
		const char* const line_y_plus  = g_game_field + (std::min(c_field_height - 2, y) + 1) * c_field_width;

		for(uint32_t x = 0; x < c_field_width ; ++x)
		{
			const char block = line[x];
			if(block == ' ')
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

			const char block_y_minus = line_y_minus[x];
			const char block_y_plus  = line_y_plus [x];
			const char block_x_minus = line[x_minus_one_clamped];
			const char block_x_plus  = line[x_plus_one_clamped ];

			// Sides.
			if(block_x_plus != ' ' && block_x_minus != ' ')
			{
				if(block_y_minus == ' ')
				{
					for(uint32_t dx = 0; dx < c_block_size; ++dx)
						set_pixel(dx, 4);
				}
				if(block_y_plus  == ' ')
				{
					for(uint32_t dx = 0; dx < c_block_size; ++dx)
						set_pixel(dx, 3);
				}
			}
			if(block_y_minus != ' ' && block_y_plus != ' ')
			{
				if(block_x_minus == ' ')
				{
					for(uint32_t dy = 0; dy < c_block_size; ++dy)
						set_pixel(4, dy);
				}
				if(block_x_plus  == ' ')
				{
					for(uint32_t dy = 0; dy < c_block_size; ++dy)
						set_pixel(3, dy);
				}
			}

			// Outer corners.
			if(block_x_minus == ' ' && block_y_minus == ' ' && block_x_plus != ' ' && block_y_plus != ' ')
			{
				set_pixel(4, 6);
				set_pixel(4, 7);
				set_pixel(6, 4);
				set_pixel(7, 4);
				set_pixel(5, 5);
			}
			if(block_x_plus == ' ' && block_y_minus == ' ' && block_x_minus != ' ' && block_y_plus != ' ')
			{
				set_pixel(3, 6);
				set_pixel(3, 7);
				set_pixel(0, 4);
				set_pixel(1, 4);
				set_pixel(2, 5);
			}
			if(block_x_minus == ' ' && block_y_plus == ' ' && block_x_plus != ' ' && block_y_minus != ' ')
			{
				set_pixel(4, 0);
				set_pixel(4, 1);
				set_pixel(6, 3);
				set_pixel(7, 3);
				set_pixel(5, 2);
			}
			if(block_x_plus == ' ' && block_y_plus == ' ' && block_x_minus != ' ' && block_y_minus != ' ')
			{
				set_pixel(3, 0);
				set_pixel(3, 1);
				set_pixel(0, 3);
				set_pixel(1, 3);
				set_pixel(2, 2);
			}

			// Inner corners.
			if(block_x_minus != ' ' && block_x_plus != ' ' && block_y_minus != ' ' && block_y_plus != ' ')
			{
				if(line_y_minus[x_minus_one_clamped ] == ' ')
				{
					set_pixel(4, 0);
					set_pixel(4, 1);
					set_pixel(4, 2);
					set_pixel(0, 4);
					set_pixel(1, 4);
					set_pixel(2, 4);
					set_pixel(3, 3);
				}
				if(line_y_plus  [x_minus_one_clamped] == ' ')
				{
					set_pixel(4, 5);
					set_pixel(4, 6);
					set_pixel(4, 7);
					set_pixel(0, 3);
					set_pixel(1, 3);
					set_pixel(2, 3);
					set_pixel(3, 4);
				}
				if(line_y_minus [x_plus_one_clamped ] == ' ')
				{
					set_pixel(3, 0);
					set_pixel(3, 1);
					set_pixel(3, 2);
					set_pixel(5, 4);
					set_pixel(6, 4);
					set_pixel(7, 4);
					set_pixel(4, 3);
				}
				if(line_y_plus  [x_plus_one_clamped ] == ' ')
				{
					set_pixel(3, 5);
					set_pixel(3, 6);
					set_pixel(3, 7);
					set_pixel(5, 3);
					set_pixel(6, 3);
					set_pixel(7, 3);
					set_pixel(4, 4);
				}
			}

		} // for x
	} // for y
}

GameInterfacePtr GamePacman::AskForNextGameTransition()
{
	return std::move(next_game_);
}

