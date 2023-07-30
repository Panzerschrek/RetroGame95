#include "SystemWindowDOS.hpp"

const uint32_t g_framebuffer_width  = 320;
const uint32_t g_framebuffer_height = 240;

SystemWindow::SystemWindow()
{
}

SystemWindow::~SystemWindow()
{
}

std::vector<SDL_Event> SystemWindow::GetEvents()
{
	std::vector<SDL_Event> events;
	return events;
}

std::vector<bool> SystemWindow::GetKeyboardState()
{
	int key_count = 255;

	std::vector<bool> result(size_t(key_count), false);
	return result;
}

void SystemWindow::BeginFrame()
{
}

FrameBuffer SystemWindow::GetFrameBuffer()
{
	FrameBuffer frame_buffer;
	frame_buffer.width  = g_framebuffer_width;
	frame_buffer.height = g_framebuffer_height;

	frame_buffer.data = nullptr;

	return frame_buffer;
}

void SystemWindow::EndFrame()
{

}
