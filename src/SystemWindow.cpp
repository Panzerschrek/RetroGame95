#include "SystemWindow.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstring>
#include <iostream>


SystemWindow::SystemWindow()
{
	// TODO - check errors.
	SDL_Init(SDL_INIT_VIDEO);

	const Uint32 window_flags= SDL_WINDOW_SHOWN;

	const int width = 640;
	const int height= 480;

	window_=
		SDL_CreateWindow(
			"RetroGame95",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			width, height,
			window_flags);

	if(window_ == nullptr)
		std::cerr << "Could not create window";
}

SystemWindow::~SystemWindow()
{
	SDL_DestroyWindow(window_);
}

std::vector<SDL_Event> SystemWindow::GetEvents()
{
	std::vector<SDL_Event> events;
	SDL_Event event;
	while( SDL_PollEvent(&event) != 0 )
	{
		events.push_back(event);
	}

	return events;
}

const SDL_Surface& SystemWindow::GetSurface() const
{
	return *surface_;
}

void SystemWindow::BeginFrame()
{
	surface_= SDL_GetWindowSurface(window_);

	if(SDL_MUSTLOCK(surface_))
		SDL_LockSurface(surface_);

	std::memset(surface_->pixels, 0, size_t(surface_->pitch * surface_->h));
}

void SystemWindow::EndFrame()
{
	if(SDL_MUSTLOCK(surface_))
		SDL_UnlockSurface(surface_);

	SDL_UpdateWindowSurface(window_);
	surface_= nullptr;
}
