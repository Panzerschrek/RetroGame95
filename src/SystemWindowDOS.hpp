#pragma once
#include "FrameBuffer.hpp"
#include <SDL.h>
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
};
