#pragma once
#include "GameInterface.hpp"
#include "GamesCommon.hpp"
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
		Brick,
		Concrete,
		Trees,
		Water,
	};

	struct Block
	{
		BlockType type : 4;
		// 0 if totally destroyed.
		uint8_t destruction_mask : 4;
	};

	struct Player
	{
		fixed16vec2_t position{};
		GridDirection direction = GridDirection::YMinus;
	};

	static const constexpr uint32_t c_field_width  = 32;
	static const constexpr uint32_t c_field_height = 26;

	static const constexpr uint32_t c_block_size = 8; // In pixels.

private:
	void ProcessPlayerInput(const std::vector<bool>& keyboard_state);

private:
	SoundPlayer& sound_player_;
	GameInterfacePtr next_game_;

	Block field_[c_field_width * c_field_height];

	std::optional<Player> player_;
};
