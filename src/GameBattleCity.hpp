#pragma once
#include "GameInterface.hpp"
#include "GamesCommon.hpp"
#include "Rand.hpp"
#include "SoundPlayer.hpp"
#include <optional>

class GameBattleCity final : public GameInterface
{
public:
	explicit GameBattleCity(SoundPlayer& sound_player);

public: // GameInterface
	virtual void Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) const override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	enum class BlockType
	{
		Empty,
		Bricks,
		Concrete,
		Foliage,
		Water,
	};

	struct Block
	{
		BlockType type : 4;
		// Bit mask of 4 blocks parts. 1 - part is alive, 0 - destroyed.
		uint8_t destruction_mask : 4;
	};

	struct Projectile
	{
		fixed16vec2_t position{};
		GridDirection direction = GridDirection::YMinus;
	};

	struct Player
	{
		fixed16vec2_t position{};
		GridDirection direction = GridDirection::YMinus;
		uint32_t next_shot_tick = 0;
		uint32_t shield_end_tick = 0;
		std::vector<Projectile> projectiles;
	};

	enum class EnemyType : uint8_t
	{
		Basic,
		LessRandom,
		Fast,
		Heavy,
		NumTypes,
	};

	struct Enemy
	{
		EnemyType type = EnemyType::Basic;
		uint8_t health = 0;
		bool gives_bonus = false;
		fixed16vec2_t position{};
		GridDirection direction = GridDirection::YPlus;
		uint32_t spawn_tick = 0;
		std::optional<Projectile> projectile;
	};

	struct PacmanGhost
	{
		fixed16vec2_t position{};
		GridDirection direction = GridDirection::YPlus;
		PacmanGhostType type = PacmanGhostType::Blinky;
	};

	struct Explosion
	{
		fixed16vec2_t position{};
		uint32_t start_tick = 0;
	};

	enum class BonusType : uint8_t
	{
		ExtraLife,
		TankUpgrade,
		Shield,
		BaseProtection,
		DestryAllTanks,
		PauseAllTanks,
		NumTypes,
	};

	struct Bonus
	{
		BonusType type = BonusType::ExtraLife;
		fixed16vec2_t position{};
	};

	struct ActiveSound
	{
		uint32_t end_tick = 0;
		SoundId id = SoundId::NumSounds;
	};

	static const constexpr uint32_t c_field_width  = 32;
	static const constexpr uint32_t c_field_height = 26;

	static const constexpr uint32_t c_block_size = 8; // In pixels.

	static constexpr const uint32_t c_base_wall_tiles[][2]
	{
		{c_field_width / 2 - 2, c_field_height - 1},
		{c_field_width / 2 - 2, c_field_height - 2},
		{c_field_width / 2 - 2, c_field_height - 3},
		{c_field_width / 2 - 1, c_field_height - 3},
		{c_field_width / 2 - 0, c_field_height - 3},
		{c_field_width / 2 + 1, c_field_height - 1},
		{c_field_width / 2 + 1, c_field_height - 2},
		{c_field_width / 2 + 1, c_field_height - 3},
	};

private:
	void EndLevel();
	void NextLevel();
	void ProcessPlayerInput(const std::vector<bool>& keyboard_state);
	void TryToPickUpBonus();

	void UpdateEnemy(Enemy& enemy);
	void UpdatePacmanGhost(PacmanGhost& pacman_ghost);

	// Returns true if need to kill it.
	bool UpdateProjectile(Projectile& projectile, bool is_player_projectile);

	bool CanMove(const fixed16vec2_t& position) const;

	void KillPlayer();

	void SpawnPlayer();
	void SpawnNewEnemy();
	void TrySpawnPacmanGhost();
	void SpawnBonus();
	void MakeExplosion(const fixed16vec2_t& position);

	void ActivateBaseProtectionBonus();
	void UpdateBaseProtectionBonus();

	void MakeEventSound(SoundId sound_id);

	void FillField(const char* field_data);
	static BlockType GetBlockTypeForLevelDataByte(char b);

	static Projectile MakeProjectile(const fixed16vec2_t& tank_position, GridDirection tank_direction);

private:
	SoundPlayer& sound_player_;

	Rand rand_;
	uint32_t tick_ = 0;

	GameInterfacePtr next_game_;

	Block field_[c_field_width * c_field_height];
	bool base_is_destroyed_ = false;

	std::optional<Player> player_;
	uint32_t player_level_ = 1; // Saved between levels, but it is reseted after death.
	std::vector<Enemy> enemies_;
	uint32_t enemies_left_ = 0;
	std::vector<PacmanGhost> pacman_ghosts_;
	std::vector<Explosion> explosions_;
	std::optional<Bonus> bonus_;
	uint32_t enemies_freezee_bonus_end_tick_ = 0;
	uint32_t base_protection_bonus_end_tick_ = 0;
	uint32_t level_start_animation_end_tick_ = 0;
	uint32_t level_end_animation_end_tick_ = 0;

	std::optional<ActiveSound> current_sound_;

	uint32_t lives_ = 0;
	uint32_t level_ = 0;
	bool game_over_ = false;
};
