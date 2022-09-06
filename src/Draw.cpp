#include "Draw.hpp"
#include <cassert>

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