#pragma once
#include "FrameBuffer.hpp"
#include "SpriteBMP.hpp"

// Draw whole sprite without borders check.
void DrawSpriteUnchecked(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint32_t start_x,
	uint32_t start_y);

// Draw whole sprite without borders check. Reject texels with transparent color.
void DrawSpriteWithAlphaUnchecked(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);
