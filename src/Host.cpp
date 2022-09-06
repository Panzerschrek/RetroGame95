#include "Host.hpp"
#include "GameMainMenu.hpp"
#include <thread>

Host::Host()
	: system_window_()
	, init_time_(Clock::now())
	, prev_tick_time_(GetCurrentTime())
	, game_(std::make_unique<GameMainMenu>())
{
}

bool Host::Loop()
{
	const auto tick_start_time = GetCurrentTime();

	if(game_ != nullptr)
	{
		if(auto next_game = game_->AskForNextGameTransition())
		{
			game_ = std::move(next_game);
		}
	}

	const auto events = system_window_.GetEvents();
	const auto keyboard_state = system_window_.GetKeyboardState();

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_QUIT)
		{
			return true;
		}
	}

	if(game_ != nullptr)
	{
		// Perform some ticks. Possible 0, 1 or many. But do not perform more than 5 ticks once.
		for (
			uint64_t
				physics_start_tick = prev_tick_time_ * GameInterface::c_update_frequency / c_time_point_resolution,
				physics_end_tick = tick_start_time * GameInterface::c_update_frequency / c_time_point_resolution,
				t = physics_start_tick,
				iterations= 0;
			t < physics_end_tick && iterations < 5;
			++t, ++iterations)
		{
			game_->Tick(events, keyboard_state);
		}
	}

	system_window_.BeginFrame();

	if(game_ != nullptr)
	{
		game_->Draw(system_window_.GetFrameBuffer());
	}

	system_window_.EndFrame();

	const TimePoint tick_end_time= GetCurrentTime();
	const auto frame_dt= tick_end_time - tick_start_time;

	const uint64_t max_fps = 120;
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
