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
	enum class PacmanDirection
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
		PacmanDirection direction = PacmanDirection::XPlus;
		PacmanDirection next_direction = PacmanDirection::XPlus;
		fixed16vec2_t target_position{};
	};

	static const constexpr uint32_t c_field_width = 33;
	static const constexpr uint32_t c_field_height = 30;
	static const constexpr uint32_t c_block_size = 8;

private:
	SoundPlayer& sound_player_;
	Rand rand_;

	uint32_t tick_ = 0;

	Pacman pacman_;

	GameInterfacePtr next_game_;
};