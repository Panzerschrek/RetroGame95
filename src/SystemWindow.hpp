#pragma once
#include "FrameBuffer.hpp"
#include <SDL_video.h>
#include <SDL_events.h>
#include <vector>


class SystemWindow
{
public:
	SystemWindow();
	~SystemWindow();

	std::vector<SDL_Event> GetEvents();
	std::vector<bool> GetKeyboardState();

	void BeginFrame();
	FrameBuffer GetFrameBuffer();
	void EndFrame();

private:
	void UpdateWindowSize();

private:
	SDL_Window* window_= nullptr;
	SDL_Surface* surface_= nullptr;
	uint32_t scale_ = 1;
	bool use_crt_effect_ = true;
	std::vector<Color32> frame_buffer_data_;
};
