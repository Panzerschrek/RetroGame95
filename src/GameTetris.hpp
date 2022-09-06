#pragma once
#include "GameInterface.hpp"
#include "Rand.hpp"
#include <array>
#include <optional>

class GameTetris final : public GameInterface
{
public:
	GameTetris();

public: // GameInterface
	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) override;

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
	void TickInternal();

	ActivePiece SpawnActivePiece();
	Block GenerateNextPieceType();

private:
	Rand rand_;

	uint32_t num_ticks_ = 0;
	uint32_t speed_ = 10; // Process actual logic each N tick

	bool game_over_ = false;

	Block field_[ c_field_width * c_field_height] {};
	std::optional<ActivePiece> active_piece_;
};
