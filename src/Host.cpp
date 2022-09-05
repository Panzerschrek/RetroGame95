#include "Sprites.hpp"
#include "SpriteBMP.hpp"
#include "Host.hpp"
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

	const SpriteBMP sprite(Sprites::gimp_harold);

	const FrameBuffer& frame_buffer = system_window_.GetFrameBuffer();

	{
		const auto w = sprite.GetWidth();
		const auto h = sprite.GetHeight();
		const auto stride = sprite.GetRowStride();
		const auto palette = sprite.GetPalette();
		const auto data = sprite.GetImageData();

		for(uint32_t y = 0; y < h; ++y)
		{
			for(uint32_t x = 0; x < w; ++x)
			{
				const auto color_index = data[x + y * stride];
				frame_buffer.data[x + y * frame_buffer.width] = palette[color_index];
			}
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
