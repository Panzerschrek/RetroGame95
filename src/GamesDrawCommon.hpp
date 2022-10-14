#pragma once
#include "FrameBuffer.hpp"
#include "GamesCommon.hpp"
#include "SpriteBMP.hpp"

const constexpr uint32_t g_arkanoid_field_offset_x = 10;
const constexpr uint32_t g_arkanoid_field_offset_y = 10;

void DrawArkanoidFieldBorder(FrameBuffer frame_buffer, bool draw_exit);

void DrawArkanoidField(FrameBuffer frame_buffer, const ArkanoidBlock* field);
void DrawArkanoidField(FrameBuffer frame_buffer, const ArkanoidBlock* field, uint32_t start_column, uint32_t end_column);

void DrawArkanoidLevelStartSplash(FrameBuffer frame_buffer, uint32_t level);
void DrawArakoindStats(FrameBuffer frame_buffer, uint32_t level, uint32_t score);

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

void DrawSnakeStats(FrameBuffer frame_buffer, uint32_t length, uint32_t lives, uint32_t level, uint32_t score);

const uint32_t g_pacman_block_size = 8;

void DrawPacmanField(
	FrameBuffer frame_buffer,
	const char* field_data,
	uint32_t field_width,
	uint32_t field_height,
	uint32_t x_start,
	uint32_t y_start,
	uint32_t x_end,
	uint32_t y_end);

SpriteBMP GetPacmanGhostSprite(PacmanGhostType ghost_type, GridDirection ghost_direction);
