#pragma once
#include <array>
#include <cstdint>

using Color32 = uint32_t;
using Color24 = std::array<uint8_t, 3>;

const constexpr Color32 g_color_black = 0;
const constexpr Color32 g_color_white = ~0u;
