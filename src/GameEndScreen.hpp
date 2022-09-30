#pragma once
#include "GameInterface.hpp"
#include "SoundPlayer.hpp"

class GameEndScreen final : public GameInterface
{
public:
	explicit GameEndScreen(SoundPlayer& sound_player);

public: // GameInterface
	virtual void Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) const override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

private:
	SoundPlayer& sound_player_;
	GameInterfacePtr next_game_;

	bool music_started_ = false;
};
