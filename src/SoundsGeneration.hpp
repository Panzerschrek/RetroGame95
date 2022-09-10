#pragma once
#include "SoundOut.hpp"
#include "Fixed.hpp"

SoundData GenSinWaveSound(uint32_t sample_rate, fixed16_t sin_wave_frequency, uint32_t periods);

// Good-sounding square waves are lying in frequency range from approximately 80Hz up to 1/4 of output frequency (~2048 for 8192 Hz output).
SoundData GetSquareWaveSound(uint32_t sample_rate, fixed16_t sin_wave_frequency, uint32_t periods);
