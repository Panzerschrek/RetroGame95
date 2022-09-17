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
