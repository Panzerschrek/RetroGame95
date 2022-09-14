#pragma once
#include "FrameBuffer.hpp"
#include <SDL_events.h>
#include <memory>
#include <vector>

class GameInterface;
using GameInterfacePtr = std::unique_ptr<GameInterface>;

class GameInterface
{
public:
	virtual ~GameInterface() = default;

	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) = 0;

	virtual void Draw(FrameBuffer frame_buffer) const = 0;

	virtual bool NeedToCaptureMouse() { return false; }

	// Returns null if should keep playing this game, returns non-null in case of transition to another game.
	virtual GameInterfacePtr AskForNextGameTransition() = 0;

	// Game implementation should return "true" in order to trigger whole application quit.
	virtual bool AskForQuit() { return false; }

public:
	// Frequency of ticks. Use fixed update step for simplicity.
	static constexpr const uint32_t c_update_frequency = 120;
};

