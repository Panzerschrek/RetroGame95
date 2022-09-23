#pragma once
#include "GameInterface.hpp"
#include "Fixed.hpp"
#include "Rand.hpp"
#include "SoundPlayer.hpp"
#include <optional>

class GamePacman final : public GameInterface
{
public:
	GamePacman(SoundPlayer& sound_player);

public: // GameInterface
	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) const override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	enum class GridDirection
	{
		XPlus,
		XMinus,
		YPlus,
		YMinus,
	};

	struct ArkanoidBallModifier
	{
		fixed16vec2_t velocity{};
		uint32_t end_tick = 0;
	};

	struct Pacman
	{
		fixed16vec2_t position{};
		// Current moving direction.
		GridDirection direction = GridDirection::XPlus;
		GridDirection next_direction = GridDirection::XPlus;
		fixed16vec2_t target_position{};

		// Non-empty if killed and dead animation is played.
		std::optional<uint32_t> dead_animation_end_tick;

		// Non-empty if is an arkanoid ball mode.
		std::optional<ArkanoidBallModifier> arkanoid_ball;

		uint32_t turret_shots_left = 0;
	};

	enum class GhostType
	{
		Blinky,
		Pinky,
		Inky,
		Clyde,
	};

	enum class GhostMode
	{
		Chase,
		Scatter,
		Frightened,
		Eaten,
	};

	struct Ghost
	{
		GhostType type = GhostType::Blinky;
		fixed16vec2_t position{};
		GridDirection direction = GridDirection::XPlus;
		fixed16vec2_t target_position{};
		GhostMode mode = GhostMode::Chase;
		uint32_t frightened_mode_end_tick = 0;
	};

	enum class Bonus
	{
		None,
		Food,
		Deadly,
		TetrisBlock0,
		TetrisBlock1,
		TetrisBlock2,
		TetrisBlock3,
		TetrisBlock4,
		TetrisBlock5,
		TetrisBlock6,
		SnakeFoodSmall,
		SnakeFoodMedium,
		SnakeFoodLarge,
		SnakeExtraLife,
	};

	static const constexpr uint32_t c_field_width = 33;
	static const constexpr uint32_t c_field_height = 30;
	static const constexpr uint32_t c_block_size = 8;

	static const constexpr uint32_t c_num_ghosts = 4;

private:
	void DrawPacman(FrameBuffer frame_buffer) const;
	void DrawGhost(FrameBuffer frame_buffer, const Ghost& ghost) const;

	void NextLevel();
	void SpawnPacmanAndGhosts();
	void MovePacman();
	void MoveGhost(Ghost& ghost);
	std::array<int32_t, 2> GetGhostDestinationBlock(
		GhostType ghost_type,
		GhostMode ghost_mode,
		const std::array<int32_t, 2>& ghost_position);
	void ProcessPacmanGhostsTouch();
	void TryTeleportCharacters();

	void PickUpBonus(Bonus& bonus);

	void UpdateGhostsMode();
	void EnterFrightenedMode();

	void TryPlaceRandomTetrisPiece();
	void TrySpawnSnakeBonus();

	static bool IsBlockInsideGhostsRoom(const std::array<int32_t, 2>& block);
	static std::array<int32_t, 2> GetScatterModeTarget(GhostType ghost_type);
	static void ReverseGhostMovement(Ghost& ghost);
	static fixed16_t GetGhostSpeed(GhostMode ghost_mode);

private:
	SoundPlayer& sound_player_;
	Rand rand_;

	uint32_t tick_ = 0;

	Pacman pacman_;
	uint32_t spawn_animation_end_tick_ = 0;
	std::array<Ghost, c_num_ghosts> ghosts_;
	Bonus bonuses_[c_field_width * c_field_height]{};
	uint32_t bonuses_left_ = 0;
	uint32_t bonuses_eaten_ = 0;
	uint32_t level_ = 0;
	uint32_t lifes_ = 3;
	uint32_t score_ = 0;
	bool game_over_ = false;

	GhostMode current_ghosts_mode_ = GhostMode::Scatter;
	uint32_t ghosts_mode_switches_left_ = 0;
	uint32_t next_ghosts_mode_swith_tick_ = 0;

	GameInterfacePtr next_game_;
};
