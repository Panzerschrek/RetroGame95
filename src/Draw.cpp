#include "Draw.hpp"
#include "String.hpp"
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

} // namespace

void FillWholeFrameBuffer(const FrameBuffer frame_buffer, const uint8_t cga_color_index)
{
	FillRect(frame_buffer, cga_color_index, 0, 0, frame_buffer.width, frame_buffer.height);
}

void FillRect(
	const FrameBuffer frame_buffer,
	const uint8_t cga_color_index,
	const uint32_t start_x,
	const uint32_t start_y,
	const uint32_t w,
	const uint32_t h)
{
	assert(start_x + w <= frame_buffer.width);
	assert(start_y + h <= frame_buffer.height);
	assert(cga_color_index < std::size(g_cga_palette));

	const Color32 color= g_cga_palette[cga_color_index];

	for(uint32_t y = 0; y < h; ++y)
	{
		const auto dst_line = frame_buffer.data + (y + start_y) * frame_buffer.width;
		for(uint32_t x = 0; x < w; ++x)
		{
			dst_line[start_x + x] = color;
		}
	}
}

void DrawSprite(
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

void DrawSpriteRect(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint32_t start_x,
	const uint32_t start_y,
	const uint32_t sprite_start_x,
	const uint32_t sprite_start_y,
	const uint32_t sprite_rect_width,
	const uint32_t sprite_rect_height)
{
	const auto h = sprite.GetHeight();
	const auto stride = sprite.GetRowStride();
	const auto palette = sprite.GetPalette();
	const auto data = sprite.GetImageData();

	assert(start_x + sprite_rect_width  <= frame_buffer.width );
	assert(start_y + sprite_rect_height <= frame_buffer.height);

	assert(sprite_start_x + sprite_rect_width  <= sprite.GetWidth ());
	assert(sprite_start_y + sprite_rect_height <= sprite.GetHeight());

	for(uint32_t y = 0; y < sprite_rect_height; ++y)
	{
		const auto src_line = data + (h - 1 - (sprite_start_y + y)) * stride;
		const auto dst_line = frame_buffer.data + (y + start_y) * frame_buffer.width;
		for(uint32_t x = 0; x < sprite_rect_width ; ++x)
		{
			const auto color_index = src_line[sprite_start_x + x];
			dst_line[start_x + x] = palette[color_index];
		}
	}
}

void DrawSpriteWithAlpha(
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

void DrawSpriteWithAlphaTransformed(
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
	DrawSpriteWithAlphaTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaMirrorX(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { -g_fixed16_one, 0, IntToFixed16(int32_t(start_x) + int32_t(sprite.GetWidth () - 1)) };
	matrix.y = { 0, -g_fixed16_one, IntToFixed16(int32_t(start_y) + int32_t(sprite.GetHeight() - 1)) };
	DrawSpriteWithAlphaTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaMirrorY(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { g_fixed16_one, 0, -IntToFixed16(int32_t(start_x)) };
	matrix.y = { 0, g_fixed16_one, -IntToFixed16(int32_t(start_y)) };
	DrawSpriteWithAlphaTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaRotate90(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { 0, g_fixed16_one, -IntToFixed16(int32_t(start_y)) };
	matrix.y = { g_fixed16_one, 0, -IntToFixed16(int32_t(start_x)) };
	DrawSpriteWithAlphaTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaRotate180(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { -g_fixed16_one, 0, IntToFixed16(int32_t(start_x) + int32_t(sprite.GetWidth() - 1)) };
	matrix.y = { 0, g_fixed16_one, -IntToFixed16(int32_t(start_y)) };
	DrawSpriteWithAlphaTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawSpriteWithAlphaRotate270(
	const FrameBuffer frame_buffer,
	const SpriteBMP sprite,
	const uint8_t transparent_color_index,
	const uint32_t start_x,
	const uint32_t start_y)
{
	Matrix3 matrix;
	matrix.x = { 0, -g_fixed16_one, IntToFixed16(int32_t(start_y) + int32_t(sprite.GetWidth () - 1)) };
	matrix.y = { -g_fixed16_one, 0, IntToFixed16(int32_t(start_x) + int32_t(sprite.GetHeight() - 1)) };
	DrawSpriteWithAlphaTransformed(frame_buffer, sprite, transparent_color_index, matrix);
}

void DrawText(
	const FrameBuffer frame_buffer,
	const uint8_t cga_color_index,
	const uint32_t start_x,
	const uint32_t start_y,
	const char* text)
{
	assert(cga_color_index < std::size(g_cga_palette));
	const Color32 color= g_cga_palette[cga_color_index];

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
		else if(code_point >= 0x2500 && code_point <= 0x257F)
		{
			glyph = &font8x8_box[code_point - 0x2500];
		}
		else if(code_point >= 0x2580 && code_point <= 0x259F)
		{
			glyph = &font8x8_block[code_point - 0x259F];
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

void DrawTextWithLightShadow(
	const FrameBuffer frame_buffer,
	const uint8_t cga_color_index,
	const uint8_t cga_color_index_shadow,
	const uint32_t start_x,
	const uint32_t start_y,
	const char* const text)
{
	DrawText(frame_buffer, cga_color_index, start_x + 1, start_y + 1, text);
	DrawText(frame_buffer, cga_color_index_shadow, start_x, start_y, text);
}

void DrawTextWithFullShadow(
	const FrameBuffer frame_buffer,
	const uint8_t cga_color_index,
	const uint8_t cga_color_index_shadow,
	const uint32_t start_x,
	const uint32_t start_y,
	const char* const text)
{
	DrawText(frame_buffer, cga_color_index_shadow, start_x + 1, start_y, text);
	DrawText(frame_buffer, cga_color_index_shadow, start_x, start_y + 1, text);
	DrawText(frame_buffer, cga_color_index_shadow, start_x + 1, start_y + 1, text);
	DrawText(frame_buffer, cga_color_index, start_x, start_y, text);
}

void DrawTextWithOutline(
	const FrameBuffer frame_buffer,
	const uint8_t cga_color_index,
	const uint8_t cga_color_index_outline,
	const uint32_t start_x,
	const uint32_t start_y,
	const char* const text)
{
	DrawText(frame_buffer, cga_color_index_outline, start_x + 1, start_y, text);
	DrawText(frame_buffer, cga_color_index_outline, start_x - 1, start_y, text);
	DrawText(frame_buffer, cga_color_index_outline, start_x, start_y + 1, text);
	DrawText(frame_buffer, cga_color_index_outline, start_x, start_y - 1, text);
	DrawText(frame_buffer, cga_color_index_outline, start_x + 1, start_y + 1, text);
	DrawText(frame_buffer, cga_color_index_outline, start_x + 1, start_y - 1, text);
	DrawText(frame_buffer, cga_color_index_outline, start_x - 1, start_y + 1, text);
	DrawText(frame_buffer, cga_color_index_outline, start_x - 1, start_y - 1, text);
	DrawText(frame_buffer, cga_color_index, start_x, start_y, text);
}

void DrawTextCentered(
	const FrameBuffer frame_buffer,
	const uint8_t cga_color_index,
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
			symbols_in_current_line = 0;
			continue;
		}

		++symbols_in_current_line;
		ExtractUTF8CodePoint(t);
	}
	max_symbols_in_line = std::max(max_symbols_in_line, symbols_in_current_line);

	DrawText(
		frame_buffer,
		cga_color_index,
		center_x - max_symbols_in_line * g_glyph_width / 2,
		center_y - num_lines * g_glyph_height / 2,
		text);
}

void DrawTextCenteredWithOutline(
	const FrameBuffer frame_buffer,
	const uint8_t cga_color_index,
	const uint8_t cga_color_index_outline,
	const uint32_t center_x,
	const uint32_t center_y,
	const char* const text)
{
	for(uint32_t dx = 0; dx < 3; ++dx)
	for(uint32_t dy = 0; dy < 3; ++dy)
	{
		DrawTextCentered(frame_buffer, cga_color_index_outline, center_x + dx - 1, center_y + dy - 1, text);
	}

	DrawTextCentered(frame_buffer, cga_color_index, center_x, center_y, text);
}
