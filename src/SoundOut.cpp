#include "SoundOut.hpp"
#include <SDL.h>
#include <cassert>

namespace
{

const SDL_AudioDeviceID g_first_valid_device_id = 2u;

int32_t NearestPowerOfTwoFloor(const int32_t x)
{
	int32_t r= 1 << 30;
	while(r > x) r>>= 1;
	return r;
}

const fixed16_t g_volume_step = g_fixed16_one / 16;

} // namespace

SoundOut::SoundOut()
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	SDL_AudioSpec requested_format{};
	SDL_AudioSpec obtained_format{};

	requested_format.channels = 1;
	requested_format.freq = 8192;
	requested_format.format = AUDIO_S8;
	requested_format.callback = AudioCallback;
	requested_format.userdata = this;

	// ~ 1 callback call per two frames (60fps)
	requested_format.samples= Uint16(NearestPowerOfTwoFloor(requested_format.freq / 30));

	int device_count = SDL_GetNumAudioDevices(0);
	// Can't get explicit devices list. Trying to use first device.
	if(device_count == -1)
	{
		device_count = 1;
	}

	// First, try to open default device.
	device_id_ = SDL_OpenAudioDevice(nullptr, 0, &requested_format, &obtained_format, 0);

	if(!(device_id_ >= g_first_valid_device_id &&
		obtained_format.channels == requested_format.channels &&
		obtained_format.format   == requested_format.format))
	{
		// Try to open other devices.
		for(int i = 0; i < device_count; i++)
		{
			const char* const device_name = SDL_GetAudioDeviceName(i, 0);

			const SDL_AudioDeviceID device_id =
				SDL_OpenAudioDevice(device_name, 0, &requested_format, &obtained_format, 0);

			if(device_id >= g_first_valid_device_id &&
				obtained_format.channels == requested_format.channels &&
				obtained_format.format   == requested_format.format)
			{
				device_id_= device_id;
				break;
			}
		}
	}

		if(device_id_ < g_first_valid_device_id)
		{
			return;
		}

	sample_rate_ = uint32_t(obtained_format.freq);

	// Run
	SDL_PauseAudioDevice(device_id_ , 0);
}

SoundOut::~SoundOut()
{
	if(device_id_ >= g_first_valid_device_id)
		SDL_CloseAudioDevice(device_id_);

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SoundOut::PlaySound(const SoundData& src_sound_data)
{
	LockChannel();

	channel_.is_active = true;
	channel_.src_sound_data = &src_sound_data;
	channel_.position_samples = 0;

	UnlockChannel();
}

void SoundOut::StopPlaying()
{
	LockChannel();

	channel_.is_active = false;

	UnlockChannel();
}

void SoundOut::SetVolume(const fixed16_t volume)
{
	volume_.store(std::max(0, std::min(volume, g_fixed16_one)));
}

void SoundOut::IncreaseVolume()
{
	SetVolume(volume_.load() + g_volume_step);
}

void SoundOut::DecreaseVolume()
{
	SetVolume(volume_.load() - g_volume_step);
}

void SDLCALL SoundOut::AudioCallback(void* const userdata, Uint8* const stream, int len_bytes)
{
	const auto self = reinterpret_cast<SoundOut*>(userdata);
	self->FillAudioBuffer(reinterpret_cast<SampleType*>(stream), uint32_t(len_bytes) / sizeof(SampleType));
}

void SoundOut::FillAudioBuffer(SampleType* const buffer, const uint32_t sample_count)
{
	// Zero buffer.
	for(uint32_t i= 0u; i < sample_count; ++i)
	{
		buffer[i]= 0;
	}

	const fixed16_t volume = volume_.load();

	if(channel_.is_active && channel_.src_sound_data != nullptr)
	{
		const uint32_t samples_to_fill=
			std::min(sample_count, uint32_t(channel_.src_sound_data->samples.size()) - channel_.position_samples);
		const SampleType* const src = channel_.src_sound_data->samples.data() + channel_.position_samples;
		for(uint32_t i= 0; i < samples_to_fill; ++i)
		{
			buffer[i] = SampleType(Fixed16FloorToInt(src[i] * volume));
		}

		channel_.position_samples += samples_to_fill;
		channel_.is_active = channel_.position_samples < uint32_t(channel_.src_sound_data->samples.size());
	}
}

void SoundOut::LockChannel()
{
	if(device_id_ >= g_first_valid_device_id)
	{
		SDL_LockAudioDevice(device_id_);
	}
}

void SoundOut::UnlockChannel()
{
	if(device_id_ >= g_first_valid_device_id)
	{
		SDL_UnlockAudioDevice(device_id_);
	}
}
