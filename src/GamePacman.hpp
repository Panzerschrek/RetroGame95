#pragma once
#include "GameInterface.hpp"
#include "Fixed.hpp"
#include "Rand.hpp"
#include "SoundPlayer.hpp"

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

	struct Pacman
	{
		fixed16vec2_t position{};
		// Current moving direction.
		GridDirection direction = GridDirection::XPlus;
		GridDirection next_direction = GridDirection::XPlus;
		fixed16vec2_t target_position{};
	};

	enum class GhostType
	{
		Blinky,
		Pinky,
		Inky,
		Clyde,
	};

	struct Ghost
	{
		GhostType type = GhostType::Blinky;
		fixed16vec2_t position{};
		GridDirection direction = GridDirection::XPlus;
		fixed16vec2_t target_position{};
	};

	enum class Bonus
	{
		None,
		Food,
		Deadly,
	};

	static const constexpr uint32_t c_field_width = 33;
	static const constexpr uint32_t c_field_height = 30;
	static const constexpr uint32_t c_block_size = 8;

	static const constexpr uint32_t c_num_ghosts = 4;

private:
	void MovePacman();
	void MoveGhost(Ghost& ghost);
	std::array<int32_t, 2> GetGhostDestinationBlock(GhostType ghost_type, const std::array<int32_t, 2>& ghost_position);
	void TryTeleportCharacters();
	static bool IsBlockInsideGhostsRoom(const std::array<int32_t, 2>& block);

private:
	SoundPlayer& sound_player_;
	Rand rand_;

	uint32_t tick_ = 0;

	Pacman pacman_;
	std::array<Ghost, c_num_ghosts> ghosts_;
	Bonus bonuses_[c_field_width * c_field_height]{};

	GameInterfacePtr next_game_;
};
