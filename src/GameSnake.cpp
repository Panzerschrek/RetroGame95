#include "GameSnake.hpp"
#include "Draw.hpp"
#include "GameMainMenu.hpp"
#include "SpriteBMP.hpp"
#include "Sprites.hpp"

GameSnake::GameSnake(SoundPlayer& sound_player)
	: sound_player_(sound_player)
	, rand_(Rand::CreateWithRandomSeed())
{
}

void GameSnake::Tick(const std::vector<SDL_Event>& events, const std::vector<bool>& keyboard_state)
{
	(void) keyboard_state;

	for(const SDL_Event& event : events)
	{
		if(event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && next_game_ == nullptr)
		{
			next_game_ = std::make_unique<GameMainMenu>(sound_player_);
		}
	}
}

void GameSnake::Draw(const FrameBuffer frame_buffer)
{
	FillWholeFrameBuffer(frame_buffer, g_color_black);

	uint32_t offset_y = 16;
	uint32_t offset_x = 16;

	const SpriteBMP tail_sprite(Sprites::snake_tail);
	DrawSpriteWithAlphaUnchecked(
		frame_buffer,
		tail_sprite,
		0,
		offset_x,
		offset_y);
	offset_y += tail_sprite.GetHeight();

	const SpriteBMP body_segment_sprite(Sprites::snake_body_segment);

	for(uint32_t i = 0; i < 10; ++i)
	{
		DrawSpriteWithAlphaUnchecked(
			frame_buffer,
			body_segment_sprite,
			0,
			offset_x,
			offset_y);
		offset_y += body_segment_sprite.GetHeight();
	}

	const SpriteBMP head_sprite(Sprites::snake_head);
	DrawSpriteWithAlphaUnchecked(
		frame_buffer,
		head_sprite,
		0,
		offset_x,
		offset_y);
}

GameInterfacePtr GameSnake::AskForNextGameTransition()
{
	return std::move(next_game_);
}
