#pragma once
#include "Color.hpp"

struct FrameBuffer
{
	uint32_t width = 0;
	uint32_t height = 0;
	Color32* data = nullptr;
};
