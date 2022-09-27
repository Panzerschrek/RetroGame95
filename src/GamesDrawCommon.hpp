#pragma once
#include "FrameBuffer.hpp"
#include "GamesCommon.hpp"

uint32_t GetTetrisFieldOffsetX(FrameBuffer frame_buffer);
uint32_t GetTetrisFieldOffsetY(FrameBuffer frame_buffer);

void DrawTetrisFieldBorder(FrameBuffer frame_buffer, bool curt_upper_segments);

void DrawTetrisNextPiece(FrameBuffer frame_buffer, TetrisBlock  next_piece);

void DrawTetrisStats(FrameBuffer frame_buffer, uint32_t level, uint32_t score);

void DrawTetrisField(
	FrameBuffer frame_buffer,
	uint32_t offset_x,
	uint32_t offset_y,
	const TetrisBlock* blocks,
	uint32_t field_width,
	uint32_t field_height);
