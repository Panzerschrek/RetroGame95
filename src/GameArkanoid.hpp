#pragma once
#include "Fixed.hpp"
#include "GameInterface.hpp"
#include "Rand.hpp"
#include <optional>

class GameArkanoid final : public GameInterface
{
public:
	GameArkanoid();

public: // GameInterface
	virtual void Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) override;

	virtual bool NeedToCaptureMouse() override { return true; }

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	// Integer coordinates are in pixels.
	// fixed16 coordinates are in pixels too, but in fixed16 format.

	enum class BlockType : uint8_t
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

	struct Block
	{
		BlockType type = BlockType::Empty;
		uint8_t health = 0;
	};

	struct Ball
	{
		// Center position.
		fixed16vec2_t position{};
		// In fixed16 pixels / tick.
		fixed16vec2_t velocity{};
		// If true - position is relative to the ship.
		bool is_attached_to_ship = false;
	};

	enum class ShipState : uint8_t
	{
		Normal,
		Sticky,
		Large,
		Turret,
	};

	struct Ship
	{
		// Center position.
		fixed16vec2_t position{};
		ShipState state = ShipState::Normal;
		uint32_t state_end_tick = 0;
		uint32_t next_shoot_tick = 0;
	};

	struct DeathAnimation
	{
		fixed16vec2_t ship_position{};
		uint32_t end_tick = 0;
	};

	enum class BonusType : uint8_t
	{
		NextLevel,
		StickyShip,
		BallSplit,
		LargeShip,
		LaserShip,
		ExtraLife,
		SlowDown,
		NumBonuses,
	};

	struct Bonus
	{
		BonusType type = BonusType::NextLevel;
		// Center position.
		fixed16vec2_t position{};
	};

	struct LaserBeam
	{
		// Center position.
		fixed16vec2_t position{};
	};

	static const constexpr uint32_t c_field_width = 11;
	static const constexpr uint32_t c_field_height = 21;

	// Size on pixels.
	static const constexpr uint32_t c_ball_half_size = 3;
	static const constexpr uint32_t c_block_width = 20;
	static const constexpr uint32_t c_block_height = 10;
	static const constexpr uint32_t c_bonus_half_width = 10;
	static const constexpr uint32_t c_bonus_half_height = 5;
	static const constexpr uint32_t c_laser_beam_width = 1;
	static const constexpr uint32_t c_laser_beam_height = 7;

	static const constexpr uint32_t c_ship_half_width_normal = 16;
	static const constexpr uint32_t c_ship_half_width_large = 24;
	static const constexpr uint32_t c_ship_half_height = 5;

private:
	void ProcessLogic(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state);
	void NextLevel();
	void SpawnShip();

	// Returns true if need to kill it.
	bool UpdateBall(Ball& ball);

	// Returns true if need to kill it.
	bool UpdateBonus(Bonus& ball);

	// Returns true if need to kill it.
	bool UpdateLaserBeam(LaserBeam& laser_beam);

	void DamageBlock(uint32_t block_x, uint32_t block_y);
	void TrySpawnNewBonus(uint32_t block_x, uint32_t block_y);

	void SplitBalls();
	void ReleaseStickyBalls();
	void CorrectShipPosition();

	static uint32_t GetShipHalfWidthForState(ShipState ship_state);

private:
	Rand rand_;
	uint32_t tick_ = 0;

	GameInterfacePtr next_game_;

	Block field_[c_field_width * c_field_height];
	std::optional<Ship> ship_;
	std::optional<DeathAnimation> death_animation_;
	bool game_over_ = false;
	std::vector<Ball> balls_;
	std::vector<Bonus> bonuses_;
	BonusType prev_bonus_type_ = BonusType::StickyShip;
	std::vector<LaserBeam> laser_beams_;
	bool next_level_exit_is_open_ = false;
	uint32_t level_start_animation_end_tick_ = 0;

	uint32_t level_ = 0;
	uint32_t lifes_ = 3;
	uint32_t score_ = 0;

	uint32_t slow_down_end_tick_ = 0;
};
