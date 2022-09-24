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
void DrawSprite(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint32_t start_x,
	uint32_t start_y);

// Draw whole sprite without borders check. Reject texels with transparent color.
void DrawSpriteWithAlpha(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaTransformed(
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

void DrawSpriteWithAlphaMirrorX(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaMirrorY(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaRotate90(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaRotate180(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

void DrawSpriteWithAlphaRotate270(
	FrameBuffer frame_buffer,
	SpriteBMP sprite,
	uint8_t transparent_color_index,
	uint32_t start_x,
	uint32_t start_y);

constexpr const uint32_t g_glyph_width  = 8;
constexpr const uint32_t g_glyph_height = 8;

void DrawText(
	FrameBuffer frame_buffer,
	Color32 color,
	uint32_t start_x,
	uint32_t start_y,
	const char* text);

void DrawTextCentered(
	FrameBuffer frame_buffer,
	Color32 color,
	uint32_t center_x,
	uint32_t center_y,
	const char* text);
