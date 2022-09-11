#pragma once
#include "GameInterface.hpp"
#include "SoundPlayer.hpp"

class GameMainMenu final : public GameInterface
{
public:
	explicit GameMainMenu(SoundPlayer& sound_player);

	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

	virtual bool AskForQuit() override;

private:
	enum class MenuRow
	{
		NewGame,
		Quit,
		NumRows,
	};

private:
	SoundPlayer& sound_player_;

	MenuRow current_row_ = MenuRow::NewGame;

	GameInterfacePtr next_game_ = nullptr;
	bool quit_triggered_ = false;
};
