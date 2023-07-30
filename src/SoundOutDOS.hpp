#pragma once
#include "Fixed.hpp"
#include "SoundOutCommon.hpp"
#include <vector>


class SoundOut final
{
public:
	SoundOut();
	~SoundOut();

	SoundOut(const SoundOut&) = delete;
	SoundOut& operator=(const SoundOut&) = delete;

	// Sound data reference must outlive this clss.
	void PlaySound(const SoundData& src_sound_data);
	void PlayLoopedSound(const SoundData& src_sound_data);
	void StopPlaying();

	uint32_t GetSampleRate() const { return sample_rate_; }

	void SetVolume(fixed16_t volume);
	void IncreaseVolume();
	void DecreaseVolume();

private:
	uint32_t sample_rate_= 22050u; // samples per second
};

