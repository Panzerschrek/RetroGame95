#include "Host.hpp"
#include "Draw.hpp"
#include "Sprites.hpp"
#include "SpriteBMP.hpp"
#include <thread>

Host::Host()
	: system_window_()
	, init_time_(Clock::now())
	, prev_tick_time_(GetCurrentTime())
{
}

bool Host::Loop()
{
	const auto tick_start_time= GetCurrentTime();

	for(const SDL_Event& event : system_window_.GetEvents())
	{
		if(event.type == SDL_QUIT)
		{
			return true;
		}
	}

	system_window_.BeginFrame();


	const FrameBuffer& frame_buffer = system_window_.GetFrameBuffer();
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::gimp_harold), 47, 16);

	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_ghost), 128, 32 );
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_1), 144, 32);
	DrawSpriteUnchecked(frame_buffer, SpriteBMP(Sprites::pacman_food), 144 + 16, 32 + 6);


	const SpriteBMP sprites[]
		{
		Sprites::tetris_block_1,
		Sprites::tetris_block_2,
		Sprites::tetris_block_3,
		Sprites::tetris_block_4,
		Sprites::tetris_block_5,
		Sprites::tetris_block_6,
		Sprites::tetris_block_7
		};
	for(uint32_t y = 0; y < 20; ++y)
	{
		for(uint32_t x = 0; x < 10; ++x)
		{
			const auto& sprite = sprites[ (x / 4 + y/2) % std::size(sprites)];
			DrawSpriteWithAlphaUnchecked(
				frame_buffer,
				sprite,
				0,
				x * sprite.GetWidth(),
				y * sprite.GetHeight());
		}
	}

	system_window_.EndFrame();

	const TimePoint tick_end_time= GetCurrentTime();
	const auto frame_dt= tick_end_time - tick_start_time;

	const uint64_t max_fps= 120;
	const auto min_frame_duration = c_time_point_resolution / max_fps;
	if(frame_dt <= min_frame_duration)
	{
		std::this_thread::sleep_for(ChronoDuration(min_frame_duration - frame_dt));
	}

	prev_tick_time_= tick_start_time;

	return false;
}

Host::TimePoint Host::GetCurrentTime()
{
	const Clock::time_point now= Clock::now();
	const auto dt= now - init_time_;

	return TimePoint(std::chrono::duration_cast<ChronoDuration>(dt).count());
}
