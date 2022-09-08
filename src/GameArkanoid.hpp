#pragma once
#include "Fixed.hpp"
#include "GameInterface.hpp"
#include <optional>

class GameArkanoid final : public GameInterface
{
public:
	GameArkanoid();

public: // GameInterface
	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) override;

	virtual bool NeedToCaptureMouse() { return true; }

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	// Integer coordinates are in pixels.
	// fixed16 coordinates are in pixels too, but in fixed16 format.

	struct Ball
	{
		// Center position.
		fixed16vec2_t position{};
		// In fixed16 pixels / tick.
		fixed16vec2_t velocity{};
	};

	struct Ship
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

	static const constexpr uint32_t c_ship_half_width_normal = 20;
	static const constexpr uint32_t c_ship_half_height = 5;


private:
	GameInterfacePtr next_game_;

	std::optional<Ship> ship_;
	std::vector<Ball> balls_;
};
