#pragma once
#include "GameInterface.hpp"

class GameArkanoid final : public GameInterface
{
public:
	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	GameInterfacePtr next_game_;
};
