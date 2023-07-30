#pragma once
#include <array>
#include <cstdint>

using Color32 = uint32_t;

const constexpr uint8_t g_color_black_index = 0;
const constexpr uint8_t g_color_white_index = 15;

using ColorComponents = std::array<uint32_t, 4>;

inline ColorComponents UnpackColor(const Color32 color)
{
	return {color >> 24, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF};
}

inline Color32 PackColor(const ColorComponents& components)
{
	return
		(std::min(0xFFu, components[0]) << 24) |
		(std::min(0xFFu, components[1]) << 16) |
		(std::min(0xFFu, components[2]) <<  8) |
		std::min(0xFFu, components[3]);
}

inline ColorComponents ColorComponentsAdd(ColorComponents c, const ColorComponents& src)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		c[i] += src[i];
	}

	return c;
}

inline ColorComponents ColorComponentsShiftLeft(ColorComponents c, const uint32_t shift)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		c[i] <<= shift;
	}

	return c;
}

inline ColorComponents ColorComponentsShiftRight(ColorComponents c, const uint32_t shift)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		c[i] >>= shift;
	}

	return c;
}

