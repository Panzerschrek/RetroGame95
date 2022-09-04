#pragma once
#include "SystemWindow.hpp"
#include <chrono>


class Host
{
public:
	Host();

	// Returns false on quit
	bool Loop();

private:
	using TimePoint = uint64_t;
	using ChronoDuration= std::chrono::milliseconds;
	static constexpr const uint64_t c_time_point_resolution= ChronoDuration::period::den; // time points in seconds

	using Clock= std::chrono::steady_clock;

private:
	TimePoint GetCurrentTime();

private:
	SystemWindow system_window_;

	const Clock::time_point init_time_;
	TimePoint prev_tick_time_;

};
