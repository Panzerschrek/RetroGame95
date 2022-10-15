#pragma once
#include "SoundOut.hpp"
#include "Fixed.hpp"

SoundData GenArkanoidBallHitSound(uint32_t sample_rate);
SoundData GenTetrisFigureStep(uint32_t sample_rate);
SoundData GenSnakeBonusEat(uint32_t sample_rate);
SoundData GenCharacterDeath(uint32_t sample_rate);
SoundData GenTankMovement(uint32_t sample_rate);
SoundData GenTankStay(uint32_t sample_rate);
SoundData GenTankShot(uint32_t sample_rate);
SoundData GenProjectileHit(uint32_t sample_rate);
SoundData GenExplosion(uint32_t sample_rate);
