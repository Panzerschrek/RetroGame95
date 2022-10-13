#pragma once
#include <array>
#include <cstdint>

using Color32 = uint32_t;
using Color24 = std::array<uint8_t, 3>;

inline constexpr const Color32 g_cga_palette[16]
{
	0x00000000,
	0x000000AA,
	0x0000AA00,
	0x0000AAAA,
	0x00AA0000,
	0x00AA00AA,
	0x00AA5500,
	0x00AAAAAA,
	0x00555555,
	0x005555FF,
	0x0055FF55,
	0x0055FFFF,
	0x00FF5555,
	0x00FF55FF,
	0x00FFFF55,
	0x00FFFFFF,
};

const constexpr Color32 g_color_black = g_cga_palette[0];
const constexpr Color32 g_color_white = g_cga_palette[15];

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

inline ColorComponents ColorComponentsScale(ColorComponents c, const uint32_t scaler)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		c[i] *= scaler;
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

inline ColorComponents ColorComponentsDiv(ColorComponents c, const uint32_t divisor)
{
	for(size_t i = 0; i < c.size(); ++i)
	{
		c[i] /= divisor;
	}

	return c;
}
