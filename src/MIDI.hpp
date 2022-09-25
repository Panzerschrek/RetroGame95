#pragma once
#include "SoundOut.hpp"

const std::vector<uint8_t> LoadMIDIFile(const char* file_name);

SoundData MakeMIDISound(const std::vector<uint8_t>& data);
