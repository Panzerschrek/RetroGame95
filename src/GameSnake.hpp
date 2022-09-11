#pragma once
#include "GameInterface.hpp"
#include "Rand.hpp"
#include "SoundPlayer.hpp"


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
	SoundPlayer& sound_player_;
	Rand rand_;

	GameInterfacePtr next_game_;
};
