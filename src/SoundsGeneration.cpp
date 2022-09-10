#include "SoundsGeneration.hpp"
#include "Rand.hpp"
#include <limits>

namespace
{

const int32_t g_sample_scale = int32_t(std::numeric_limits<SampleType>::max());
const float g_two_pi = 2.0f * 3.1415926535f;

} // namespace

SoundData GenSinWaveSound(const uint32_t sample_rate, const fixed16_t sin_wave_frequency, const uint32_t periods)
{
	const uint32_t total_samples =
		uint32_t(uint64_t(sample_rate) * uint64_t(periods) * uint64_t(g_fixed16_one) / uint64_t(sin_wave_frequency));

	SoundData out_data;
	out_data.samples.resize(total_samples);

	const float scale_f = float(sin_wave_frequency) * g_two_pi / (float(sample_rate) * float(g_fixed16_one));
	const float sample_scale = float(g_sample_scale);
	for(uint32_t i= 0; i < total_samples; ++i)
	{
		out_data.samples[i] = SampleType(std::sin(float(i) * scale_f) * sample_scale);
	}

	return out_data;
}

SoundData GetSquareWaveSound(const uint32_t sample_rate, const fixed16_t sin_wave_frequency, const uint32_t periods)
{
	const uint32_t total_samples =
		uint32_t(uint64_t(sample_rate) * uint64_t(periods) * uint64_t(g_fixed16_one) / uint64_t(sin_wave_frequency));

	SoundData out_data;
	out_data.samples.resize(total_samples);

	// TODO - maybe make smooth square wave to imitate imperfection of PC-speaker?
	const fixed16_t scale = 2 * sin_wave_frequency / int32_t(sample_rate);
	for(uint32_t i = 0; i < total_samples; ++i)
	{
		out_data.samples[i] =
			SampleType((((int32_t(i) * scale) >> g_fixed16_base) & 1) == 0 ? g_sample_scale : (-g_sample_scale));
	}

	return out_data;
}
