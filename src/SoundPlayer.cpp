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
		GenTankMovement,
		GenTankStay,
		GenTankShot,
		GenProjectileHit,
		GenExplosion,
	};

	for(size_t i= 0; i < size_t(SoundId::NumSounds); ++i)
	{
		sounds_[i] = c_gen_funcs[i](sound_out_.GetSampleRate());
	}

	static const constexpr std::pair<const uint8_t*, size_t> c_music_data[]
	{
		{Music::in_taberna, std::size(Music::in_taberna)},
		{Music::herr_mannelig, std::size(Music::herr_mannelig)},
		{Music::ritt_der_toten, std::size(Music::ritt_der_toten)},
		{Music::du_hast_den_farbfilm_vergessen, std::size(Music::du_hast_den_farbfilm_vergessen)},
		{Music::in_meinem_raum, std::size(Music::in_meinem_raum)},
		{Music::heavy_metal, std::size(Music::heavy_metal)},
		{Music::preussens_gloria, std::size(Music::preussens_gloria)},
	};

	for(size_t i= 0; i < size_t(MusicId::NumMelodies); ++i)
	{
		music_[i] = MakeMIDISound(c_music_data[i].first, c_music_data[i].second, sound_out_.GetSampleRate());
	}
}

void SoundPlayer::PlaySound(const SoundId sound_id)
{
	sound_out_.PlaySound(sounds_[size_t(sound_id)]);
}

void SoundPlayer::PlayLoopedSound(const SoundId sound_id)
{
	sound_out_.PlayLoopedSound(sounds_[size_t(sound_id)]);
}

void SoundPlayer::PlayMusic(const MusicId music_id)
{
	sound_out_.PlaySound(music_[size_t(music_id)]);
}

void SoundPlayer::StopPlaying()
{
	sound_out_.StopPlaying();
}

fixed16_t SoundPlayer::GetMelodyDuration(const MusicId music_id) const
{
	return fixed16_t((int64_t(music_[size_t(music_id)].samples.size()) << g_fixed16_base) / int64_t(sound_out_.GetSampleRate()));
}
