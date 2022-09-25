#include "SoundPlayer.hpp"
#include "SoundsGeneration.hpp"
#include "MIDI.hpp"
#include "Music.hpp"

SoundPlayer::SoundPlayer(SoundOut& sound_out)
	: sound_out_(sound_out)
{
	using GenFunc= SoundData(*)(uint32_t frequency);
	static constexpr GenFunc c_gen_funcs[size_t(SoundId::NumSounds)]
	{
		GenArkanoidBallHitSound,
		GenTetrisFigureStep,
		GenSnakeBonusEat,
		GenCharacterDeath,
	};

	for(size_t i= 0; i < size_t(SoundId::NumSounds); ++i)
	{
		sounds_[i] = c_gen_funcs[i](sound_out_.GetSampleRate());
	}

	const std::pair<const uint8_t*, size_t> c_music_data[]
	{
		{Music::test, std::size(Music::test)},
	};

	for(size_t i= 0; i < size_t(MusicId::NumMelidies); ++i)
	{
		music_[i] = MakeMIDISound(c_music_data[i].first, c_music_data[i].second, sound_out_.GetSampleRate());
	}
}

void SoundPlayer::PlaySound(const SoundId sound_id)
{
	sound_out_.PlaySound(sounds_[size_t(sound_id)]);
}

void SoundPlayer::PlayMusic(const MusicId music_id)
{
	sound_out_.PlaySound(music_[size_t(music_id)]);
}

void SoundPlayer::StopPlaying()
{
	sound_out_.StopPlaying();
}
