#include "MIDI.hpp"
#include <iostream>
#include <optional>

namespace
{

#pragma pack(push, 1)
struct MIDIHeader
{
	char type[4];
	uint32_t length;
	uint16_t format;
	uint16_t number_of_tracks;
	int16_t division;
};

static_assert(sizeof(MIDIHeader) == 14, "Invalid size");

struct TrackHeader
{
	char type[4];
	uint32_t length;
};

#pragma pack(pop)


struct ChannelState
{
	uint32_t num_presses = 0;
	uint32_t note_number = 0;
};

using Channels = std::array<ChannelState, 16>;

uint32_t ByteSwap(const uint32_t x)
{
	return
		((x & 0x000000FF) << 24) |
		((x & 0x0000FF00) <<  8) |
		((x & 0x00FF0000) >>  8) |
		((x & 0xFF000000) >> 24);
}

uint16_t ByteSwap(const uint16_t x)
{
	return
		((x & 0x00FF) << 8) |
		((x & 0xFF00) >> 8);
}

uint32_t ReadVarLen(const uint8_t* const data, size_t& offset)
{
	uint32_t value = 0;
	do
	{
		const uint8_t b = data[offset];
		++offset;
		value = (value << 7) | (b & 0x7F);

		if((b & 0x80) == 0)
		{
			break;
		}
	}while(true);

	return value;
}

void FillSoundData(const Channels& channels, const uint32_t sample_rate, const uint32_t duration, SoundData& sound_data)
{
	sound_data.samples.resize(sound_data.samples.size() + duration, 0);

	std::optional<uint32_t> note;
	for(const ChannelState& channel : channels)
	{
		if(channel.num_presses > 0)
		{
			note = channel.note_number;
			break;
		}
	}
	if(note == std::nullopt)
	{
		return;
	}

	// note A4
	const float base_freq = 440.0f;
	const uint32_t base_freq_note = 12 * 4 + 9;

	const float mul = std::pow(2.0f, 1.0f / 12.0f);

	const float freq = base_freq * std::pow( mul, float(*note - base_freq_note) );
	const float freq_scaled = freq * (2.0f * 3.1415926535f) / float(sample_rate);

	for(size_t i = sound_data.samples.size() - duration; i < sound_data.samples.size(); ++i)
	{
		sound_data.samples[i] = SampleType(127.0f * std::sin(freq_scaled * float(i)));
	}
}

SoundData LoadTrack(const uint8_t* data, const size_t data_size, const uint32_t sample_rate, const float time_scaler)
{
	const auto header = reinterpret_cast<const TrackHeader*>(data);

	SoundData result;
	if(std::strncmp(header->type, "MTrk", 4) != 0)
	{
		return result;
	}
	const uint32_t length = ByteSwap(header->length);
	if(length > data_size)
	{
		return result;
	}

	Channels channels;

	size_t offset = sizeof(TrackHeader);
	bool first_event = true;
	while(offset < data_size)
	{
		const uint32_t delta_time = ReadVarLen(data, offset);
		const uint8_t event = data[offset];
		++offset;

		const uint32_t delta_samples = uint32_t(float(delta_time) * time_scaler * float(sample_rate));
		std::cout << "Delta samples " << delta_samples << std::endl;
		if(!first_event)
		{
			FillSoundData(channels, sample_rate, delta_samples, result);
		}
		first_event = false;

		const uint8_t event_type = (event >> 4);
		ChannelState& channel = channels[event & 15];

		switch (event_type)
		{
		case 0x8:
			{
				const uint8_t note_number = data[offset];
				offset += 2;
				std::cout << "Channel " << uint32_t(event & 15) << " Note release " << uint32_t(note_number) << std::endl;
				if(channel.num_presses > 0)
				{
					channel.num_presses -= 1;
					if(channel.num_presses == 0)
					{
						channel.note_number = 0;
					}
				}
			}
			break;

		case 0x9:
			{
				const uint8_t note_number = data[offset];
				std::cout << "Channel " << uint32_t(event & 15) << " Note press " << uint32_t(note_number) << std::endl;
				channel.num_presses += 1;
				channel.note_number = std::max(channel.note_number, uint32_t(note_number));
				offset += 2;
			}
			break;

		case 0xA:
			std::cout << "Polyphonic Key Pressure" << std::endl;
			offset += 2;
			break;

		case 0xB:
			std::cout << "Control change" << std::endl;
			offset += 2;
			break;

		case 0xC:
			std::cout << "Program Change" << std::endl;
			++offset;
			break;

		case 0xD:
			std::cout << "Channel Pressure (After-touch)." << std::endl;
			++offset;
			break;

		case 0xE:
			std::cout << "Pitch Wheel Change." << std::endl;
			offset += 2;
			break;

		case 0xF:
			if(event == 0xFF)
			{
				const uint8_t meta_event = data[offset];
				++offset;

				const uint32_t meta_length = ReadVarLen(data, offset);
				offset += meta_length;
				std::cout << "Meta event " << uint32_t(meta_event) << " with length " << meta_length << std::endl;
			}
			else
			{
				const uint32_t meta_length = ReadVarLen(data, offset);
				offset += meta_length;
				std::cout << "Event " << uint32_t(event) << " With length " << meta_length << std::endl;
			}
			break;

		default:
			std::cout << "Unrecognized event." << std::endl;
			break;
		}
		if(event < 0x80)
		{
			std::cout << "Strange event" << std::endl;
			offset += 1;
		}
	}

	std::cout << "Result size: " << result.samples.size() << std::endl;
	return result;
}

} // namespace

const std::vector<uint8_t> LoadMIDIFile(const char* file_name)
{
	FILE* const f = std::fopen(file_name, "rb");

	std::fseek( f, 0, SEEK_END );
	const unsigned int file_size = std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	std::vector<uint8_t> out_file_content;
	out_file_content.resize( file_size );
	std::fread(out_file_content.data(), 1, file_size, f);

	std::fclose( f );
	return out_file_content;
}

SoundData MakeMIDISound(const std::vector<uint8_t>& data, const uint32_t sample_rate)
{
	SoundData result;

	const auto header = reinterpret_cast<const MIDIHeader*>(data.data());
	if(std::strncmp(header->type, "MThd", 4) != 0)
	{
		return result;
	}
	const uint32_t length = ByteSwap(header->length);
	if(length != 6)
	{
		return result;
	}

	const auto format = ByteSwap(header->format);
	if(format != 0)
	{
		return result;
	}

	const int16_t division = ByteSwap(uint16_t(header->division));

	const float time_scaler = 0.5f / float(division);

	return LoadTrack(data.data() + sizeof(MIDIHeader), data.size() - sizeof(MIDIHeader), sample_rate, time_scaler);
}
