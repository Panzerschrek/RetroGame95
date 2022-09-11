#pragma once
#include <vector>
#include <SDL_audio.h>

using SampleType = int8_t;

struct SoundData
{
	std::vector<SampleType> samples;
};

class SoundOut final
{
public:
	SoundOut();
	~SoundOut();

	SoundOut(const SoundOut&) = delete;
	SoundOut& operator=(const SoundOut&) = delete;

	// Sound data reference must outlive this clss.
	void PlaySound(const SoundData& src_sound_data);
	void StopPlaying();

	uint32_t GetSampleRate() const { return sample_rate_; }

private:
	static void SDLCALL AudioCallback(void* userdata, Uint8* stream, int len_bytes);
	void FillAudioBuffer(SampleType* buffer, uint32_t sample_count);

	void LockChannel();
	void UnlockChannel();

private:
	struct Channel
	{
		bool is_active = false;
		uint32_t position_samples = 0;
		const SoundData* src_sound_data = nullptr;
	};

private:
	SDL_AudioDeviceID device_id_ = 0u;
	uint32_t sample_rate_= 0u; // samples per second

	// This struct is protected via mutex.
	Channel channel_;
};

