#pragma once
#include "FrameBuffer.hpp"
#include "Matrix.hpp"
#include "SpriteBMP.hpp"

void FillWholeFrameBuffer(FrameBuffer frame_buffer, Color32 color);

void FillRect(
	FrameBuffer frame_buffer,
	Color32 color,
	uint32_t start_x,
	uint32_t start_y,
	uint32_t w,
	uint32_t h);

void DrawHorisontalLine(FrameBuffer frame_buffer, Color32 color, uint32_t start_x, uint32_t start_y, uint32_t length);

void DrawVerticaLine(FrameBuffer frame_buffer, Color32 color, uint32_t start_x, uint32_t start_y, uint32_t length);

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

void DrawSpriteWithAlphaUncheckedTransformed(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	const Matrix3& tc_matrix);

void DrawSpriteWithAlphaIdentityTransform(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaUncheckedMirrorX(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaUncheckedMirrorY(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaUncheckedRotate90(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaUncheckedRotate180(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaUncheckedRotate270(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawText(
	FrameBuffer frame_buffer,
	Color32 color,
	uint32_t start_x,
	uint32_t start_y,
	const char* text);
