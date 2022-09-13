#pragma once
#include "GameInterface.hpp"
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

	virtual void Draw(FrameBuffer frame_buffer) override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	struct SnakeSegment
	{
		std::array<uint32_t, 2> position{};
	};

	enum class SnakeDirection
	{
		XPlus,
		XMinus,
		YPlus,
		YMinus,
	};

	struct Snake
	{
		// Head segment has index 0.
		std::vector<SnakeSegment> segments;
		SnakeDirection direction = SnakeDirection::XPlus;
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

	static const constexpr uint32_t c_field_width = 30;
	static const constexpr uint32_t c_field_height = 20;
	static const constexpr uint32_t c_block_size = 10;

	static const constexpr uint32_t c_num_bonuses = 3;

private:
	void NextLevel();
	void NewField();
	void SpawnSnake();
	void MoveSnake();
	bool IsPositionFree(const std::array<uint32_t, 2>& position) const;

	std::array<uint32_t, 2> GetRandomPosition();
	std::array<uint32_t, 2> GetRandomFreePosition();

	Bonus SpawnBonus();

private:
	SoundPlayer& sound_player_;
	Rand rand_;

	uint32_t tick_ = 0;

	std::optional<Snake> snake_;
	std::array<Bonus, c_num_bonuses> bonuses_;
	uint32_t grow_points_ = 0;
	std::optional<uint32_t> death_animation_end_tick_; // Non-empty if is dead.
	std::optional<uint32_t> field_start_animation_end_tick_; // Non-empty if just started and show game field.
	uint32_t lifes_ = 3;
	uint32_t level_ = 0;
	uint32_t score_ = 0;
	bool game_over_ = false;

	GameInterfacePtr next_game_;
};
