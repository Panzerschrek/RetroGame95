#pragma once
#include <SDL_video.h>
#include <SDL_events.h>
#include <vector>


class SystemWindow
{
public:
	SystemWindow();
	~SystemWindow();

	std::vector<SDL_Event> GetEvents();

	const SDL_Surface& GetSurface() const;

	void BeginFrame();
	void EndFrame();

private:
	SDL_Window* window_= nullptr;
	SDL_Surface* surface_= nullptr;
};
