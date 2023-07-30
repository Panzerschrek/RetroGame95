#include "GameEndScreen.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "Sprites.hpp"
#include "Strings.hpp"

GameEndScreen::GameEndScreen(SoundPlayer& sound_player)
	: sound_player_(sound_player)
{
	OpenGame(GameId::EndScreen);
}

void GameEndScreen::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void) keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
	}

	if(!music_started_)
	{
		music_started_ = true;
		sound_player_.PlayMusic(MusicId::InTaberna);
	}
}

void GameEndScreen::Draw(const FrameBuffer frame_buffer) const
{
	DrawSprite(frame_buffer, Sprites::dresdner_zwinger_in_der_nacht, 0, 0);

	DrawTextWithOutline(
		frame_buffer,
		11,
		0,
		g_glyph_width / 2,
		g_glyph_height * 1,
		Strings::end_screen_congratulations);

	DrawTextWithOutline(
		frame_buffer,
		15,
		0,
		g_glyph_width / 2,
		g_glyph_height * 10,
		Strings::end_screen_authors);

	DrawTextWithFullShadow(
		frame_buffer,
		14,
		8,
		frame_buffer.width - 20 * g_glyph_width,
		frame_buffer.height - g_glyph_height * 4 - g_glyph_height / 2,
		Strings::end_screen_peace_text);
}

GameInterfacePtr GameEndScreen::AskForNextGameTransition()
{
	return std::move(next_game_);
}
