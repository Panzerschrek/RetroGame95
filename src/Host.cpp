#include "Host.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "SoundsGeneration.hpp"
#include "Strings.hpp"
#include <thread>

Host::Host()
	: system_window_()
	, sound_out_()
	, sound_player_(sound_out_)
	, init_time_(Clock::now())
	, prev_tick_time_(GetCurrentTime())
	, game_(std::make_unique<GameMainMenu>(sound_player_))
{
	sound_player_.PlayMusic(MusicId::HerrMannelig);
}

bool Host::Loop()
{
	const auto tick_start_time = GetCurrentTime();

	// Perform some ticks. Possible 0, 1 or many. But do not perform more than 5 ticks at once.
	for (
		uint64_t
			physics_start_tick = prev_tick_time_ * GameInterface::c_update_frequency / c_time_point_resolution,
			physics_end_tick = tick_start_time * GameInterface::c_update_frequency / c_time_point_resolution,
			t = physics_start_tick,
			iterations= 0;
		game_ != nullptr && t < physics_end_tick && iterations < 5;
		++t, ++iterations)
	{
		const auto events = system_window_.GetEvents();
		const auto keyboard_state = system_window_.GetKeyboardState();

		for(const SDL_Event& event : events)
		{
			if(event.type == SDL_QUIT)
			{
				return true;
			}
			if(event.type == SDL_KEYDOWN)
			{
				if(event.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET)
				{
					sound_out_.DecreaseVolume();
				}
				if(event.key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET)
				{
					sound_out_.IncreaseVolume();
				}
				if(event.key.keysym.scancode == SDL_SCANCODE_PAUSE)
				{
					paused_ = !paused_;
				}
				if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && paused_)
				{
					paused_ = false;
				}
			}
		}

		if(game_->AskForQuit())
		{
			return true;
		}
		if(auto next_game = game_->AskForNextGameTransition())
		{
			game_ = std::move(next_game);
			sound_out_.StopPlaying();
		}

		SDL_SetRelativeMouseMode((!paused_ && game_->NeedToCaptureMouse()) ? SDL_TRUE : SDL_FALSE);

		if(!paused_)
		{
			game_->Tick(events, keyboard_state);
		}
	} // For game logic iterations.

	system_window_.BeginFrame();

	const FrameBuffer frame_buffer = system_window_.GetFrameBuffer();
	if(game_ != nullptr)
	{
		game_->Draw(frame_buffer);
	}

	if(paused_)
	{
		const uint8_t colors[2] = {8, 15};
		const uint32_t index = prev_tick_time_ / 300 % 2;

		DrawTextCenteredWithOutline(
			frame_buffer,
			g_cga_palette[colors[index]],
			g_cga_palette[colors[index ^ 1]],
			frame_buffer.width  / 2,
			frame_buffer.height / 2,
			Strings::paused);
	}

	system_window_.EndFrame();

	const TimePoint tick_end_time = GetCurrentTime();
	const auto frame_dt = tick_end_time - tick_start_time;

	const uint64_t max_fps = 120;
	const auto min_frame_duration = c_time_point_resolution / max_fps;
	if(frame_dt <= min_frame_duration)
	{
		std::this_thread::sleep_for(ChronoDuration(min_frame_duration - frame_dt));
	}

	prev_tick_time_ = tick_start_time;

	return false;
}

Host::TimePoint Host::GetCurrentTime()
{
	const Clock::time_point now = Clock::now();
	const auto dt = now - init_time_;

	return TimePoint(std::chrono::duration_cast<ChronoDuration>(dt).count());
}
