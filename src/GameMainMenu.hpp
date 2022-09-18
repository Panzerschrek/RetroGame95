#pragma once
#include "GameInterface.hpp"
#include "Progress.hpp"
#include "SoundPlayer.hpp"
#include <variant>

class GameMainMenu final : public GameInterface
{
public:
	explicit GameMainMenu(SoundPlayer& sound_player);

	virtual void Tick(
		const std::vector<SDL_Event>& events,
		const std::vector<bool>& keyboard_state) override;

	virtual void Draw(FrameBuffer frame_buffer) const override;

	virtual GameInterfacePtr AskForNextGameTransition() override;

	virtual bool AskForQuit() override;

private:
	enum class MenuRow
	{
		NewGame,
		ContinueGame,
		SelectGame,
		Quit,
		NumRows,
	};

	using SelectGameMenuRow = GameId;

private:
	SoundPlayer& sound_player_;

	const Progress progress_;

	std::variant<MenuRow, SelectGameMenuRow> current_row_ = MenuRow::NewGame;

	GameInterfacePtr next_game_ = nullptr;
	bool quit_triggered_ = false;
};
