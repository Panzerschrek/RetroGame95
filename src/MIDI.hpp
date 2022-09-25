#pragma once
#include "SoundOut.hpp"

SoundData MakeMIDISound(const uint8_t* data, size_t data_size, uint32_t sample_rate);
