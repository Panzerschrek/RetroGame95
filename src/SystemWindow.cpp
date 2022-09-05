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

	// This resolution is close to 320x200 from CGA/EGA, but has 1:1 pixel aspect ratio.
	const uint32_t base_width = 320;
	const uint32_t base_height= 240;
	scale_ = 3;

	window_=
		SDL_CreateWindow(
			"RetroGame95",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			int(base_width * scale_), int(base_height * scale_),
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

void SystemWindow::BeginFrame()
{
	surface_= SDL_GetWindowSurface(window_);
}

FrameBuffer SystemWindow::GetFrameBuffer()
{
	FrameBuffer frame_buffer;
	frame_buffer.width = surface_->w / scale_;
	frame_buffer.height = surface_->h / scale_;

	frame_buffer_data_.resize(frame_buffer.width * frame_buffer.height, 0);
	frame_buffer.data = frame_buffer_data_.data();

	return frame_buffer;
}

void SystemWindow::EndFrame()
{
	if(SDL_MUSTLOCK(surface_))
		SDL_LockSurface(surface_);

	const FrameBuffer frame_buffer = GetFrameBuffer();

	// TODO - optimize this.
	const auto pixels = reinterpret_cast<Color32*>(surface_->pixels);
	const uint32_t dst_stride = surface_->pitch / sizeof(Color32);
	for(uint32_t y= 0; y < frame_buffer.height; ++y)
	{
		for(uint32_t x= 0; x < frame_buffer.width; ++x)
		{
			const Color32 c = frame_buffer.data[x + y * frame_buffer.width];
			for(uint32_t dy = 0; dy < scale_; ++dy)
			{
				const uint32_t dst_y = y * scale_ + dy;
				for(uint32_t dx = 0; dx < scale_; ++dx)
				{
					const uint32_t dst_x = x * scale_ + dx;
					pixels[dst_x + dst_y * dst_stride] = c;
				}
			}
		}
	}

	if(SDL_MUSTLOCK(surface_))
		SDL_UnlockSurface(surface_);

	SDL_UpdateWindowSurface(window_);
	surface_= nullptr;
}
