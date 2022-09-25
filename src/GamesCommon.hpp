#pragma once
#include "Fixed.hpp"

//
// Tetris stuff
//

constexpr uint32_t g_tetris_piece_num_blocks = 4;
constexpr uint32_t g_tetris_num_piece_types = 7;

// X and Y
using TetrisPieceBlock = std::array<int32_t, 2>;
using TetrisPieceBlocks = std::array<TetrisPieceBlock, g_tetris_piece_num_blocks>;

inline constexpr std::array<TetrisPieceBlocks, g_tetris_num_piece_types> g_tetris_pieces_blocks =
{{
	{{ { 4, -4}, {4, -3}, {4, -2}, {4, -1} }}, // I
	{{ { 4, -1}, {5, -1}, {5, -2}, {5, -3} }}, // J
	{{ { 5, -1}, {4, -1}, {4, -2}, {4, -3} }}, // L
	{{ { 4, -2}, {5, -2}, {4, -1}, {5, -1} }}, // O
	{{ { 4, -1}, {5, -1}, {5, -2}, {6, -2} }}, // S
	{{ { 4, -1}, {6, -1}, {5, -1}, {5, -2} }}, // T
	{{ { 5, -1}, {6, -1}, {5, -2}, {4, -2} }}, // Z
}};

enum class TetrisBlock : uint8_t
{
	Empty,
	I,
	J,
	L,
	O,
	S,
	T,
	Z,
};

struct TetrisPiece
{
	TetrisBlock type = TetrisBlock::I;
	// Signerd coordinate to allow apperiance form screen top.
	TetrisPieceBlocks blocks;
};

TetrisPieceBlocks RotateTetrisPieceBlocks(const TetrisPiece& piece);

// Returns true if detected collision.
bool MakeCollisionBetweenObjectAndBox(
	const fixed16vec2_t& box_min,
	const fixed16vec2_t& box_max,
	const fixed16vec2_t& object_half_size,
	fixed16vec2_t& object_position,
	fixed16vec2_t& object_velocity);

// Replacement for "snprintf". Do not use it, because it is too heavy.
void NumToString(char* str, size_t str_len, uint32_t num, uint32_t target_num_digits);
