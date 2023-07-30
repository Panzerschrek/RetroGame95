#include "Draw.hpp"
#include "String.hpp"

// Platform-independent draw functions.

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

void FillWholeFrameBuffer(const FrameBuffer frame_buffer, const uint8_t cga_color_index)
{
	FillRect(frame_buffer, cga_color_index, 0, 0, frame_buffer.width, frame_buffer.height);
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
