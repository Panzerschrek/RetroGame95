#pragma once
#include "GameInterface.hpp"
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
	static const constexpr uint32_t c_field_width = 33;
	static const constexpr uint32_t c_field_height = 30;
	static const constexpr uint32_t c_block_size = 8;

private:
	SoundPlayer& sound_player_;
	Rand rand_;

	uint32_t tick_ = 0;

	std::array<uint32_t, 2> pacman_position_{};

	GameInterfacePtr next_game_;
};
