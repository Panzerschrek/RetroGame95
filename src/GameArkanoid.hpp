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
	static const constexpr uint32_t c_field_width = 11;
	static const constexpr uint32_t c_field_height = 19;

private:
	GameInterfacePtr next_game_;
};
