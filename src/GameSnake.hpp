#pragma once
#include "GameInterface.hpp"
#include "GamesCommon.hpp"
#include "Rand.hpp"
#include "SoundPlayer.hpp"
#include <array>
#include <optional>
#include <vector>

class GameSnake final : public GameInterface
{
public:
	GameSnake(SoundPlayer& sound_player);

public: // GameInterface
	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) const override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	struct SnakeSegment
	{
		std::array<uint32_t, 2> position{};
	};

	struct Snake
	{
		// Head segment has index 0.
		std::vector<SnakeSegment> segments;
		GridDirection direction = GridDirection::XPlus;
		uint32_t grow_points_ = 0;
	};

	enum class BonusType
	{
		FoodSmall,
		FoodMedium,
		FoodLarge,
		ExtraLife,
		NumTypes,
	};

	struct Bonus
	{
		std::array<uint32_t, 2> position{ 9999, 9999};
		BonusType type = BonusType::FoodSmall;
	};

	struct ArkanoidBall
	{
		// Center position (in blocks).
		fixed16vec2_t position{};
		// In fixed16 blocks / tick.
		fixed16vec2_t velocity{};

		uint32_t bounces_left = 0;
	};

	static const constexpr uint32_t c_field_width = 30;
	static const constexpr uint32_t c_field_height = 20;
	static const constexpr uint32_t c_block_size = 10;

	static const constexpr uint32_t c_num_bonuses = 3;

private:
	void EndLevel();
	void NextLevel();
	void NewField();
	void SpawnSnake();
	void MoveSnake();
	void MoveSnakeAsTetrisPiece();
	void ManipulateSnakeAsTetrisPiece(const std::vector<SDL_Event>& events);
	void OnSnakeDeath();
	void MoveTetrisPieceDown();
	// Returns true if need to kill it.
	bool UpdateArkanoidBall(ArkanoidBall& arkanoid_ball);
	bool IsPositionFree(const std::array<uint32_t, 2>& position) const;

	std::array<uint32_t, 2> GetRandomPosition();
	std::array<uint32_t, 2> GetRandomFreePosition();

	Bonus SpawnBonus();

	void TrySpawnTetrisPiece();
	void TrySpawnArkanoidBall();

private:
	SoundPlayer& sound_player_;
	Rand rand_;

	uint32_t tick_ = 0;

	std::optional<Snake> snake_;
	std::array<Bonus, c_num_bonuses> bonuses_;
	std::optional<uint32_t> death_animation_end_tick_; // Non-empty if is dead.
	std::optional<uint32_t> field_start_animation_end_tick_; // Non-empty if just started and show game field.
	std::optional<uint32_t> level_end_animation_end_tick_; // Non-empty if playing level end animation.
	uint32_t lifes_ = 2;
	uint32_t level_ = 0;
	uint32_t score_ = 0;
	bool game_over_ = false;

	TetrisBlock tetris_field_[ c_field_width * c_field_height] {};
	std::optional<TetrisPiece> tetris_active_piece_;

	std::vector<ArkanoidBall> arkanoid_balls_;

	GameInterfacePtr next_game_;
};
