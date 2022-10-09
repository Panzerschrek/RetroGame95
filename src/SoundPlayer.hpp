#pragma once
#include "SoundOut.hpp"
#include <array>

enum class SoundId
{
	ArkanoidBallHit,
	TetrisFigureStep,
	SnakeBonusEat,
	CharacterDeath,
	NumSounds,
};

enum class MusicId
{
	InTaberna,
	HerrMannelig,
	RittDerToten,
	DuHastDenFarbfilmVergessen,
	InMeinemRaum,
	HeavyMetal,
	PreussensGloria,
	NumMelodies,
};

class SoundPlayer
{
public:
	SoundPlayer(SoundOut& sound_out);

	SoundPlayer(const SoundPlayer&) = delete;
	SoundPlayer& operator=(const SoundPlayer&) = delete;

	void PlaySound(SoundId sound_id);
	void PlayMusic(MusicId music_id);
	void StopPlaying();

	fixed16_t GetMelodyDuration(MusicId music_id) const;

private:
	SoundOut& sound_out_;
	std::array<SoundData, size_t(SoundId::NumSounds)> sounds_;
	std::array<SoundData, size_t(MusicId::NumMelodies)> music_;
};
