#pragma once
#include "SoundOut.hpp"
#include "Fixed.hpp"

SoundData GenSinWaveSound(uint32_t sample_rate, fixed16_t sin_wave_frequency, uint32_t periods);

// Good-sounding square waves are lying in frequency range from approximately 80Hz up to 1/4 of output frequency (~2048 for 8192 Hz output).
SoundData GenSquareWaveSound(uint32_t sample_rate, fixed16_t sin_wave_frequency, uint32_t periods);

SoundData GenArkanoidBallHitSound(uint32_t sample_rate);
SoundData GenTetrisFigureStep(uint32_t sample_rate);
SoundData GenSnakeBonusEat(uint32_t sample_rate);
SoundData GenCharacterDeath(uint32_t sample_rate);
SoundData GenTankMovement(uint32_t sample_rate);
SoundData GenTankStay(uint32_t sample_rate);
SoundData GenTankShot(uint32_t sample_rate);
SoundData GenProjectileHit(uint32_t sample_rate);
SoundData GenExplosion(uint32_t sample_rate);
