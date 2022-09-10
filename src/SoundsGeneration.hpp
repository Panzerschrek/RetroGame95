#pragma once
#include "SoundOut.hpp"
#include "Fixed.hpp"

SoundData GenSinWaveSound(uint32_t sample_rate, fixed16_t sin_wave_frequency, uint32_t periods);
SoundData GetSquareWaveSound(uint32_t sample_rate, fixed16_t sin_wave_frequency, uint32_t periods);
