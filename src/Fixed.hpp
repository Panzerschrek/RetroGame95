#pragma once
#include <array>
#include <cstdint>

using fixed16_t = int32_t;

using fixed16vec2_t = std::array<fixed16_t, 2>;
using fixed16vec3_t = std::array<fixed16_t, 3>;

const int32_t g_fixed16_base = 16;

const fixed16_t g_fixed16_one= 1 << 16;

inline int32_t Fixed16FloorToInt(const fixed16_t f)
{
	return f >> g_fixed16_base;
}

inline int32_t Fixed16RoundToInt(const fixed16_t f)
{
	return (f + (1 << (g_fixed16_base - 1))) >> g_fixed16_base;
}

inline int32_t Fixed16CeilToInt(const fixed16_t f)
{
	return (f + (g_fixed16_one - 1)) >> g_fixed16_base;
}

inline fixed16_t IntToFixed16(const int32_t i)
{
	return i << g_fixed16_base;
}

inline fixed16_t Fixed16Mul(const fixed16_t l, const fixed16_t r)
{
	return fixed16_t((int64_t(l) * int64_t(r)) >> g_fixed16_base);
}

inline int32_t Fixed16MulResultToInt(const fixed16_t l, const fixed16_t r)
{
	return fixed16_t((int64_t(l) * int64_t(r)) >> (g_fixed16_base * 2));
}

inline fixed16_t Fixed16Div(const fixed16_t l, const fixed16_t r)
{
	return fixed16_t((int64_t(l) << g_fixed16_base) / int64_t(r));
}

inline fixed16_t Fixed16Invert(const fixed16_t x)
{
	return fixed16_t((int64_t(1) << (g_fixed16_base * 2)) / int64_t(x));
}

inline fixed16_t Fixed16Abs(const fixed16_t x)
{
	return x >= 0 ? x : -x;
}

// result= x * y / z
// It is more precise than Fixed16Div(Fixed16Mul(x, y), z).
inline fixed16_t Fixed16MulDiv(const fixed16_t x, const int32_t y, const int32_t z)
{
	return fixed16_t(int64_t(x) * int64_t(y) / int64_t(z));
}

inline fixed16_t Fixed16VecDot(const fixed16vec2_t& v0, const fixed16vec2_t& v1)
{
	return fixed16_t((int64_t(v0[0]) * int64_t(v1[0]) + int64_t(v0[1]) * int64_t(v1[1])) >> g_fixed16_base);
}

inline fixed16_t Fixed16VecSquareLen(const fixed16vec2_t& v)
{
	return Fixed16VecDot(v, v);
}

inline int64_t Fixed16VecSquareLenScaled(const fixed16vec2_t& v)
{
	return int64_t(v[0]) * int64_t(v[0]) + int64_t(v[1]) * int64_t(v[1]);
}
