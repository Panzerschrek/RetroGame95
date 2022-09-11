#pragma once
#include "SoundOut.hpp"
#include <array>

enum class SoundId
{
	ArkanoidBallHit,
	TetrisFigureStep,
	NumSounds,
};

class SoundPlayer
{
public:
	SoundPlayer(SoundOut& sound_out);

	SoundPlayer(const SoundPlayer&) = delete;
	SoundPlayer& operator=(const SoundPlayer&) = delete;

	void PlaySound(SoundId sound_id);
	void StopPlaying();

private:
	SoundOut& sound_out_;
	std::array<SoundData, size_t(SoundId::NumSounds)> sounds_;
};
