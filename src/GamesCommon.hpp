#pragma once
#include "Fixed.hpp"

//
// Arkanoid stuff
//

enum class ArkanoidBlockType : uint8_t
{
	Empty,
	Color1,
	Color2,
	Color3,
	Color4,
	Color5,
	Color6,
	Color7,
	Color8,
	Color9,
	Color10,
	Color11,
	Color12,
	Color13,
	Color14,
	Color15,
	Concrete,
	Color14_15,
	NumTypes,
};

struct ArkanoidBlock
{
	ArkanoidBlockType type = ArkanoidBlockType::Empty;
	uint8_t health = 0;
};

const constexpr uint32_t g_arkanoid_block_width  = 20;
const constexpr uint32_t g_arkanoid_block_height = 10;

constexpr const uint32_t g_arkanoid_field_width  = 11;
constexpr const uint32_t g_arkanoid_field_height = 21;

const fixed16_t g_arkanoid_ship_keyboard_move_sensetivity = g_fixed16_one * 3 / 2; // TODO - make this configurable.
const fixed16_t g_arkanoid_ship_mouse_move_sensetivity = g_fixed16_one / 3; // TODO - make this configurable.

void FillArkanoidField(ArkanoidBlock* field, const char* field_data);

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

constexpr const uint32_t g_tetris_field_width  = 10;
constexpr const uint32_t g_tetris_field_height = 20;

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
