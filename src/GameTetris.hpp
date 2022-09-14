#pragma once
#include "GameInterface.hpp"
#include "Rand.hpp"
#include "SoundPlayer.hpp"
#include <array>
#include <optional>

class GameTetris final : public GameInterface
{
public:
	GameTetris(SoundPlayer& sound_player);

public: // GameInterface
	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) const override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	enum class Block : uint8_t
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

	static const constexpr uint32_t c_field_width = 10;
	static const constexpr uint32_t c_field_height = 20;

	struct ActivePiece
	{
		Block type = Block::I;
		// Signerd coordinate to allow apperiance form screen top.
		std::array<std::array<int32_t, 2>, 4> blocks;
	};

private:
	void NextLevel();
	void ManipulatePiece(const std::vector<SDL_Event>& events);
	void MovePieceDown();
	void UpdateScore(uint32_t lines_removed);

	ActivePiece SpawnActivePiece();
	void GenerateNextPieceType();

private:
	SoundPlayer& sound_player_;

	Rand rand_;

	uint32_t num_ticks_ = 0;
	uint32_t score_= 0;
	uint32_t level_ = 0;
	uint32_t lines_removed_for_this_level_ = 0;
	bool game_over_ = false;

	Block field_[ c_field_width * c_field_height] {};
	std::optional<ActivePiece> active_piece_;
	Block next_piece_type_ = Block::Empty;

	uint32_t pieces_spawnded_ = 0;

	GameInterfacePtr next_game_;
};
