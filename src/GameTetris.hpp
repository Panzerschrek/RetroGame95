#pragma once
#include "Fixed.hpp"
#include "GameInterface.hpp"
#include "GamesCommon.hpp"
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

	virtual bool NeedToCaptureMouse() override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	static const constexpr uint32_t c_field_width  = g_tetris_field_width;
	static const constexpr uint32_t c_field_height = g_tetris_field_height;

	enum class BonusType : uint8_t
	{
		NextLevel,
		ArkanoidBallsSpawn,
		IPiece,
		LaserShip,
		SlowDown,
		NumBonuses,
	};

	struct Bonus
	{
		BonusType type = BonusType::SlowDown;
		// Center position (in blocks).
		fixed16vec2_t position{};
	};

	struct ArkanoidBall
	{
		// Center position (in blocks).
		fixed16vec2_t position{};
		// In fixed16 blocks / tick.
		fixed16vec2_t velocity{};
	};

	struct LaserBeam
	{
		// Center position.
		fixed16vec2_t position{};
	};

	struct ArkanoidShip
	{
		// Center position.
		fixed16vec2_t position{};
	};

	static const constexpr uint32_t c_arkanoid_ship_half_width = 16;

private:
	void ProcessLogic(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state);
	void EndLevel();
	void NextLevel();
	void ProcessShootRequest();
	void ManipulatePiece(const std::vector<SDL_Event>& events);
	void MovePieceDown();
	void TryMoveWholeFieldDown();
	void TryRemoveLines();
	void UpdateScore(uint32_t lines_removed);

	// Returns true if need to kill it.
	bool UpdateArkanoidBall(ArkanoidBall& arkanoid_ball);

	// Returns true if need to kill it.
	bool UpdateBonus(Bonus& ball);

	// Returns true if need to kill it.
	bool UpdateLaserBeam(LaserBeam& laser_beam);

	void TrySpawnNewBonus(int32_t x, int32_t y);
	void TrySpawnRandomArkanoidBall();
	void SpawnArkanoidBall();

	void CorrectArkanoidShipPosition();

	TetrisPiece SpawnActivePiece();
	void GenerateNextPieceType();

private:
	SoundPlayer& sound_player_;

	Rand rand_;

	uint32_t tick_ = 0;
	uint32_t score_= 0;
	uint32_t level_ = 0;
	uint32_t lines_removed_for_this_level_ = 0;
	bool game_over_ = false;

	TetrisBlock field_[ c_field_width * c_field_height] {};
	std::optional<TetrisPiece> active_piece_;
	TetrisBlock next_piece_type_ = TetrisBlock::Empty;
	uint32_t i_pieces_left_ = 0;

	std::vector<ArkanoidBall> arkanoid_balls_;
	std::vector<Bonus> bonuses_;
	BonusType prev_bonus_type_ = BonusType::ArkanoidBallsSpawn;
	std::vector<LaserBeam> laser_beams_;
	uint32_t slow_down_end_tick_ = 0;
	uint32_t laser_ship_end_tick_ = 0;
	uint32_t next_shoot_tick_ = 0;
	bool end_level_triggered_ = false;
	uint32_t level_end_animation_end_tick_ = 0;

	uint32_t pieces_spawnded_ = 0;

	// Used for transition animation.
	ArkanoidBlock temp_arkanoid_field_[g_arkanoid_field_width * g_arkanoid_field_height];
	ArkanoidShip temp_arkanoid_ship_;

	GameInterfacePtr next_game_;
};
