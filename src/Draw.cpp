#include "Draw.hpp"
#include <cassert>

namespace
{

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wc++11-narrowing"

#endif // __GNUC__
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4309)
#pragma warning(disable : 4838)
#endif // _MSC_VER

#include "../font8x8/font8x8.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

uint32_t ExtractUTF8CodePoint(const char*& ptr)
{
	// c_bit_masks[4] - menas save first 4 bits
	static const uint32_t c_bit_masks[9]=
	{
		(1 << 0) - 1,
		(1 << 1) - 1,
		(1 << 2) - 1,
		(1 << 3) - 1,
		(1 << 4) - 1,
		(1 << 5) - 1,
		(1 << 6) - 1,
		(1 << 7) - 1,
		(1 << 8) - 1,
	};

	const char c= *ptr;
	uint32_t code = 0;

	if( ( c & 0b10000000 ) == 0 )
	{
		code = uint32_t(c);
		++ptr;
	}
	else if( ( c & 0b11100000 ) == 0b11000000 )
	{
		code=
			( (uint32_t(ptr[0]) & c_bit_masks[5]) << 6u ) |
			( (uint32_t(ptr[1]) & c_bit_masks[6]) << 0u );

		ptr += 2;
	}
	else if( ( c & 0b11110000 ) == 0b11100000 )
	{
		code=
			( (uint32_t(ptr[0]) & c_bit_masks[4]) << 12u ) |
			( (uint32_t(ptr[1]) & c_bit_masks[6]) <<  6u ) |
			( (uint32_t(ptr[2]) & c_bit_masks[6]) <<  0u );

		ptr += 3;
	}
	else if( ( c & 0b11111000 ) == 0b11110000 )
	{
		code=
			( (uint32_t(ptr[0]) & c_bit_masks[3]) << 18u ) |
			( (uint32_t(ptr[1]) & c_bit_masks[6]) << 12u ) |
			( (uint32_t(ptr[2]) & c_bit_masks[6]) <<  6u ) |
			( (uint32_t(ptr[3]) & c_bit_masks[6]) <<  0u );

		ptr += 4;
	}
	else
	{
		// Codes above unicode range - wtf?
		++ptr;
	}

	return code;
}

const uint32_t g_glyph_width = 8;
const uint32_t g_glyph_height = 8;

} // namespace

void FillWholeFrameBuffer(const FrameBuffer frame_buffer, const Color32 color)
{
	FillRect(frame_buffer, color, 0, 0, frame_buffer.width, frame_buffer.height);
}

void FillRect(
	const FrameBuffer frame_buffer,
	const Color32 color,
	const uint32_t start_x,
	const uint32_t start_y,
	const uint32_t w,
	const uint32_t h)
{
	assert(start_x + w <= frame_buffer.width);
	assert(start_y + h <= frame_buffer.height);

	for(uint32_t y = 0; y < h; ++y)
	{
		const auto dst_line = frame_buffer.data + (y + start_y) * frame_buffer.width;
		for(uint32_t x = 0; x < w; ++x)
		{
			dst_line[start_x + x] = color;
		}
	}
}

void DrawHorisontalLine(
	const FrameBuffer frame_buffer,
	const Color32 color,
	const uint32_t start_x,
	const uint32_t start_y,
	const uint32_t length)
{
	assert(start_x + length <= frame_buffer.width);
	assert(start_y <= frame_buffer.height);

	for(uint32_t x = start_x; x < start_x + length; ++x)
	{
		frame_buffer.data[x + start_y * frame_buffer.width ] = color;
	}
}

void DrawVerticaLine(
	const FrameBuffer frame_buffer,
	const Color32 color,
	const uint32_t start_x,
	const uint32_t start_y,
	const uint32_t length)
{
	assert(start_x <= frame_buffer.width);
	assert(start_y + length <= frame_buffer.height);

	for(uint32_t y = start_y; y < start_y + length; ++y)
	{
		frame_buffer.data[start_x + y * frame_buffer.width ] = color;
	}
}

void DrawSpriteUnchecked(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint32_t start_x,
	const uint32_t start_y)
{
	const auto w = sprite.GetWidth();
	const auto h = sprite.GetHeight();
	const auto stride = sprite.GetRowStride();
	const auto palette = sprite.GetPalette();
	const auto data = sprite.GetImageData();

	assert(start_x + w <= frame_buffer.width);
	assert(start_y + h <= frame_buffer.height);

	for(uint32_t y = 0; y < h; ++y)
	{
		const auto src_line = data + (h - 1 - y) * stride;
		const auto dst_line = frame_buffer.data + (y + start_y) * frame_buffer.width;
		for(uint32_t x = 0; x < w; ++x)
		{
			const auto color_index = src_line[x];
			dst_line[start_x + x] = palette[color_index];
		}
	}
}

void DrawSpriteWithAlphaUnchecked(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	const auto w = sprite.GetWidth();
	const auto h = sprite.GetHeight();
	const auto stride = sprite.GetRowStride();
	const auto palette = sprite.GetPalette();
	const auto data = sprite.GetImageData();

	assert(start_x + w <= frame_buffer.width);
	assert(start_y + h <= frame_buffer.height);

	for(uint32_t y = 0; y < h; ++y)
	{
		const auto src_line = data + (h - 1 - y) * stride;
		const auto dst_line = frame_buffer.data + (y + start_y) * frame_buffer.width;
		for(uint32_t x = 0; x < w; ++x)
		{
			const auto color_index = src_line[x];
			if(color_index != transparent_color_index)
			{
				dst_line[start_x + x] = palette[color_index];
			}
		}
	}
}

void DrawSpriteWithAlphaUncheckedTransformed(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const Matrix3& tc_matrix)
{
	const auto w = sprite.GetWidth();
	const auto h = sprite.GetHeight();
	const auto stride = sprite.GetRowStride();
	const auto palette = sprite.GetPalette();
	const auto data = sprite.GetImageData();

	const Matrix3 tc_matrix_inverse = tc_matrix.GetInverse();

	const int32_t corner_points[4][2]=
	{
		{0, 0}, {0, int32_t(h)}, {int32_t(w), 0}, {int32_t(w), int32_t(h)},
	};

	const fixed16_t tc_inf = 32767 * g_fixed16_one;
	fixed16_t min_coord[2] = {  tc_inf,  tc_inf };
	fixed16_t max_coord[2] = { -tc_inf, -tc_inf };

	for(const auto& point : corner_points)
	{
		const int32_t x = point[0] * tc_matrix_inverse.x[0] + point[1] * tc_matrix_inverse.x[1] + tc_matrix_inverse.x[2];
		const int32_t y = point[0] * tc_matrix_inverse.y[0] + point[1] * tc_matrix_inverse.y[1] + tc_matrix_inverse.y[2];
		min_coord[0] = std::min(min_coord[0], x);
		min_coord[1] = std::min(min_coord[1], y);
		max_coord[0] = std::max(max_coord[0], x);
		max_coord[1] = std::max(max_coord[1], y);
	}

	const uint32_t start_x = std::min(uint32_t(std::max(Fixed16FloorToInt(min_coord[0]) - 1, 0)), frame_buffer.width );
	const uint32_t end_x   = std::min(uint32_t(std::max(Fixed16FloorToInt(max_coord[0]) + 1, 0)), frame_buffer.width );
	const uint32_t start_y = std::min(uint32_t(std::max(Fixed16FloorToInt(min_coord[1]) - 1, 0)), frame_buffer.height);
	const uint32_t end_y   = std::min(uint32_t(std::max(Fixed16FloorToInt(max_coord[1]) + 1, 0)), frame_buffer.height);

	for(uint32_t y = start_y; y < end_y; ++y)
	{
		const auto dst_line = frame_buffer.data + y * frame_buffer.width;
		for(uint32_t x = start_x; x < end_x; ++x)
		{
			const int32_t tc_x = Fixed16FloorToInt(int32_t(x) * tc_matrix.x[0] + int32_t(y) * tc_matrix.x[1] + tc_matrix.x[2]);
			const int32_t tc_y = Fixed16FloorToInt(int32_t(x) * tc_matrix.y[0] + int32_t(y) * tc_matrix.y[1] + tc_matrix.y[2]);
			if(tc_x >= 0 && tc_y >= 0 && tc_x < int32_t(w) && tc_y < int32_t(h))
			{
				const auto color_index = data[uint32_t(tc_x) + uint32_t(tc_y) * stride];
				if(color_index != transparent_color_index)
				{
					dst_line[x] = palette[color_index];
				}
			}
		}
	}
}

void DrawSpriteWithAlphaIdentityTransform(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { g_fixed16_one, 0, -IntToFixed16(int32_t(start_x)) };
	matrix.y = { 0, -g_fixed16_one, IntToFixed16(int32_t(start_y) + int32_t(sprite.GetHeight() - 1)) };
	DrawSpriteWithAlphaUncheckedTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaUncheckedMirrorX(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { -g_fixed16_one, 0, IntToFixed16(int32_t(start_x) + int32_t(sprite.GetWidth () - 1)) };
	matrix.y = { 0, -g_fixed16_one, IntToFixed16(int32_t(start_y) + int32_t(sprite.GetHeight() - 1)) };
	DrawSpriteWithAlphaUncheckedTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaUncheckedMirrorY(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { g_fixed16_one, 0, -IntToFixed16(int32_t(start_x)) };
	matrix.y = { 0, g_fixed16_one, -IntToFixed16(int32_t(start_y)) };
	DrawSpriteWithAlphaUncheckedTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaUncheckedRotate90(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { 0, g_fixed16_one, -IntToFixed16(int32_t(start_y)) };
	matrix.y = { g_fixed16_one, 0, -IntToFixed16(int32_t(start_x)) };
	DrawSpriteWithAlphaUncheckedTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaUncheckedRotate180(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { -g_fixed16_one, 0, IntToFixed16(int32_t(start_x) + int32_t(sprite.GetWidth() - 1)) };
	matrix.y = { 0, g_fixed16_one, -IntToFixed16(int32_t(start_y)) };
	DrawSpriteWithAlphaUncheckedTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaUncheckedRotate270(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { 0, -g_fixed16_one, IntToFixed16(int32_t(start_y) + int32_t(sprite.GetWidth () - 1)) };
	matrix.y = { -g_fixed16_one, 0, IntToFixed16(int32_t(start_x) + int32_t(sprite.GetHeight() - 1)) };
	DrawSpriteWithAlphaUncheckedTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawText(
	const FrameBuffer frame_buffer,
	const Color32 color,
	const uint32_t start_x,
	const uint32_t start_y,
	const char* text)
{
	uint32_t x = start_x;
	uint32_t y = start_y;
	while(*text != '\0')
	{
		if(*text == '\n')
		{
			++text;
			x = start_x;
			y += g_glyph_height;
			continue;
		}

		const uint32_t code_point = ExtractUTF8CodePoint(text);

		auto glyph = &font8x8_basic[uint32_t('?')];
		if (code_point < 0x80)
		{
			glyph = &font8x8_basic[code_point];
		}
		else if(code_point < 0x100)
		{
			glyph = &font8x8_ext_latin[code_point - 0xA0];
		}

		for(uint32_t dy = 0; dy < g_glyph_height; ++dy)
		{
			const uint32_t dst_y = y + dy;
			const char glyph_line_byte = (*glyph)[dy];

			for(uint32_t dx = 0; dx < g_glyph_width; ++dx)
			{
				if((glyph_line_byte & (1 << dx)) != 0)
				{
					const uint32_t dst_x = x + dx;
					frame_buffer.data[ dst_x + dst_y * frame_buffer.width ] = color;
				}
			}
		}

		x+= g_glyph_width;
	}
}

void DrawTextCentered(
	const FrameBuffer frame_buffer,
	const Color32 color,
	const uint32_t center_x,
	const uint32_t center_y,
	const char* const text)
{
	uint32_t num_lines = 1;
	uint32_t max_symbols_in_line = 0;
	uint32_t symbols_in_current_line = 0;
	const char* t = text;
	while(*t != '\0')
	{
		if(*t == '\n')
		{
			max_symbols_in_line = std::max(max_symbols_in_line, symbols_in_current_line);
			++t;
			if(*t != '\0')
			{
				++num_lines;
			}
			continue;
		}

		++symbols_in_current_line;
		ExtractUTF8CodePoint(t);
	}
	max_symbols_in_line = std::max(max_symbols_in_line, symbols_in_current_line);

	DrawText(
		frame_buffer,
		color,
		center_x - max_symbols_in_line * g_glyph_width / 2,
		center_y - num_lines * g_glyph_height / 2,
		text);
}
