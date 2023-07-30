#pragma once
#include "Color.hpp"

struct FrameBuffer
{
	uint32_t width = 0;
	uint32_t height = 0;

#ifdef VERMISCHUNG_DOS
	uint8_t* data = nullptr;
#else
	Color32* data = nullptr;
#endif
};
