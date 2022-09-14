#include "SoundPlayer.hpp"
#include "SoundsGeneration.hpp"

SoundPlayer::SoundPlayer(SoundOut& sound_out)
	: sound_out_(sound_out)
{
	using GenFunc= SoundData(*)(uint32_t frequency);
	static constexpr GenFunc c_gen_funcs[size_t(SoundId::NumSounds)]
	{
		GenArkanoidBallHitSound,
		GenTetrisFigureStep,
		GenSnakeBonusEat,
		GenSnakeDeath,
	};

	for(size_t i= 0; i < size_t(SoundId::NumSounds); ++i)
	{
		sounds_[i] = c_gen_funcs[i](sound_out_.GetSampleRate());
	}
}

void SoundPlayer::PlaySound(const SoundId sound_id)
{
	sound_out_.PlaySound(sounds_[size_t(sound_id)]);
}

void SoundPlayer::StopPlaying()
{
	sound_out_.StopPlaying();
}
