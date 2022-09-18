#include "SystemWindow.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstring>

namespace
{

template<uint32_t scale>
void CopyImageWithScaleImpl(
	const Color32* src,
	const uint32_t src_width,
	const uint32_t src_height,
	Color32* const dst,
	const uint32_t dst_stride)
{
	for(uint32_t y= 0; y < src_height; ++y)
	{
		const Color32* const src_line = src + y * src_width;
		Color32* dst_start_line = dst + y * scale * dst_stride;
		for(uint32_t x= 0; x < src_width; ++x)
		{
			const Color32 c = src_line[x];
			for(uint32_t dy = 0; dy < scale; ++dy)
			{
				Color32* dst_line = dst_start_line + dy * dst_stride;
				for(uint32_t dx = 0; dx < scale; ++dx)
				{
					const uint32_t dst_x = x * scale + dx;
					dst_line[dst_x] = c;
				}
			}
		}
	}
}

constexpr const uint32_t g_max_scale = 6;
// This resolution is close to 320x200 from CGA/EGA, but has 1:1 pixel aspect ratio.
const uint32_t g_framebuffer_width  = 320;
const uint32_t g_framebuffer_height = 240;

void CopyImageWithScale(
	const uint32_t scale,
	const Color32* src,
	const uint32_t src_width,
	const uint32_t src_height,
	Color32* const dst,
	const uint32_t dst_stride)
{
	auto func = CopyImageWithScaleImpl<1>;
	switch(scale)
	{
	case 1: func = CopyImageWithScaleImpl<1>; break;
	case 2: func = CopyImageWithScaleImpl<2>; break;
	case 3: func = CopyImageWithScaleImpl<3>; break;
	case 4: func = CopyImageWithScaleImpl<4>; break;
	case 5: func = CopyImageWithScaleImpl<5>; break;
	case 6: func = CopyImageWithScaleImpl<6>; break;
	default: func = CopyImageWithScaleImpl<g_max_scale>; break;
	}

	func(src, src_width, src_height, dst, dst_stride);
}

} // namespace

SystemWindow::SystemWindow()
{
	// TODO - check errors.
	SDL_Init(SDL_INIT_VIDEO);

	const Uint32 window_flags= SDL_WINDOW_SHOWN;

	scale_ = 3;

	window_=
		SDL_CreateWindow(
			"RetroGame95",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			int(g_framebuffer_width * scale_), int(g_framebuffer_height * scale_),
			window_flags);
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
		if(event.type == SDL_KEYDOWN)
		{
			if(event.key.keysym.scancode == SDL_SCANCODE_MINUS)
			{
				if(scale_ > 1)
				{
					--scale_;
					UpdateWindowSize();
				}
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_EQUALS)
			{
				if(scale_ < g_max_scale)
				{
					++scale_;
					UpdateWindowSize();
				}
			}
		}

		events.push_back(event);
	}

	return events;
}

std::vector<bool> SystemWindow::GetKeyboardState()
{
	int key_count = 0;
	const Uint8* const keyboard_state = SDL_GetKeyboardState(&key_count);

	std::vector<bool> result(size_t(key_count), false);

	for(size_t i = 0; i < size_t(key_count); ++i)
	{
		result[i] = keyboard_state[i] != 0;
	}

	return result;
}

void SystemWindow::BeginFrame()
{
	surface_= SDL_GetWindowSurface(window_);
}

FrameBuffer SystemWindow::GetFrameBuffer()
{
	FrameBuffer frame_buffer;
	frame_buffer.width = uint32_t(surface_->w) / scale_;
	frame_buffer.height = uint32_t(surface_->h) / scale_;

	frame_buffer_data_.resize(frame_buffer.width * frame_buffer.height, 0);
	frame_buffer.data = frame_buffer_data_.data();

	return frame_buffer;
}

void SystemWindow::EndFrame()
{
	if(SDL_MUSTLOCK(surface_))
		SDL_LockSurface(surface_);

	CopyImageWithScale(
		scale_,
		frame_buffer_data_.data(),
		uint32_t(surface_->w) / scale_,
		uint32_t(surface_->h) / scale_,
		reinterpret_cast<Color32*>(surface_->pixels),
		uint32_t(surface_->pitch) / sizeof(Color32));

	if(SDL_MUSTLOCK(surface_))
		SDL_UnlockSurface(surface_);

	SDL_UpdateWindowSurface(window_);
	surface_= nullptr;
}

void SystemWindow::UpdateWindowSize()
{
	SDL_SetWindowSize(window_, int(g_framebuffer_width * scale_), int(g_framebuffer_height * scale_));
}
