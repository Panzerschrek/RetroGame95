#include "SoundsGeneration.hpp"
#include "Rand.hpp"
#include <limits>

namespace
{

// Good-sounding square waves are lying in frequency range from approximately 80Hz up to 1/4 of output frequency (~2048 for 8192 Hz output).
SoundData GenSquareWaveSound(const uint32_t sample_rate, const fixed16_t sin_wave_frequency, const uint32_t periods)
{
	const uint32_t total_samples =
		uint32_t(uint64_t(sample_rate) * uint64_t(periods) * uint64_t(g_fixed16_one) / uint64_t(sin_wave_frequency));

	SoundData out_data;
	out_data.samples.resize(total_samples);

	// TODO - maybe make smooth square wave to imitate imperfection of PC-speaker?
	const auto sample_scale = int32_t(std::numeric_limits<SampleType>::max());
	const fixed16_t scale = 2 * sin_wave_frequency / int32_t(sample_rate);
	for(uint32_t i = 0; i < total_samples; ++i)
	{
		out_data.samples[i] =
			SampleType((((int32_t(i) * scale) >> g_fixed16_base) & 1) == 0 ? sample_scale : (-sample_scale));
	}

	return out_data;
}

SoundData Concat(SoundData l, const SoundData& r)
{
	l.samples.insert(l.samples.end(), r.samples.begin(), r.samples.end());
	return l;
}

SoundData Concat(SoundData a, const SoundData& b, const SoundData& c)
{
	return Concat(Concat(a, b), c);
}

SoundData Concat(SoundData a, const SoundData& b, const SoundData& c, const SoundData& d)
{
	return Concat(Concat(a, b), c, d);
}

SoundData Concat(SoundData a, const SoundData& b, const SoundData& c, const SoundData& d, const SoundData& e)
{
	return Concat(Concat(a, b), c, d, e);
}

} // namespace

SoundData GenArkanoidBallHitSound(const uint32_t sample_rate)
{
	return GenSquareWaveSound(sample_rate, 32 * g_fixed16_one, 3);
}

SoundData GenTetrisFigureStep(const uint32_t sample_rate)
{
	return GenSquareWaveSound(sample_rate, 120 * g_fixed16_one, 6);
}

SoundData GenSnakeBonusEat(const uint32_t sample_rate)
{
	return Concat(
		GenSquareWaveSound(sample_rate, 120 * g_fixed16_one, 8),
		GenSquareWaveSound(sample_rate, 140 * g_fixed16_one, 8),
		GenSquareWaveSound(sample_rate, 160 * g_fixed16_one, 8));
}

SoundData GenCharacterDeath(const uint32_t sample_rate)
{
	return Concat(
		GenSquareWaveSound(sample_rate, 180 * g_fixed16_one, 16),
		GenSquareWaveSound(sample_rate, 160 * g_fixed16_one, 24),
		GenSquareWaveSound(sample_rate, 140 * g_fixed16_one, 32),
		GenSquareWaveSound(sample_rate, 120 * g_fixed16_one, 48));
}

SoundData GenTankMovement(const uint32_t sample_rate)
{
	return GenSquareWaveSound(sample_rate, 24 * g_fixed16_one, 8);
}

SoundData GenTankStay(const uint32_t sample_rate)
{
	return
		Concat(
			GenSquareWaveSound(sample_rate, 24 * g_fixed16_one, 3),
			GenSquareWaveSound(sample_rate, 20 * g_fixed16_one, 3));
}

SoundData GenTankShot(const uint32_t sample_rate)
{
	return Concat(
		GenSquareWaveSound(sample_rate, 900 * g_fixed16_one, 20),
		GenSquareWaveSound(sample_rate, 800 * g_fixed16_one, 20),
		GenSquareWaveSound(sample_rate, 700 * g_fixed16_one, 20));
}

SoundData GenProjectileHit(const uint32_t sample_rate)
{
	return Concat(
		GenSquareWaveSound(sample_rate, 60 * g_fixed16_one, 4),
		GenSquareWaveSound(sample_rate, 40 * g_fixed16_one, 4));
}

SoundData GenExplosion(const uint32_t sample_rate)
{
	return Concat(
		GenSquareWaveSound(sample_rate, 1200 * g_fixed16_one, 40),
		GenSquareWaveSound(sample_rate, 1500 * g_fixed16_one, 40),
		GenSquareWaveSound(sample_rate,  750 * g_fixed16_one, 60),
		GenSquareWaveSound(sample_rate,  500 * g_fixed16_one, 60),
		GenSquareWaveSound(sample_rate,  200 * g_fixed16_one, 20));
}
