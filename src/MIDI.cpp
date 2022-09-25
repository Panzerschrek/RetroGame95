#include "MIDI.hpp"
#include <iostream>

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

SoundData LoadTrack(const uint8_t* data, size_t data_size)
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

	size_t offset = sizeof(TrackHeader);
	while(offset < data_size)
	{
		const uint32_t delta_time = ReadVarLen(data, offset);
		const uint8_t event = data[offset];
		++offset;

		const uint8_t event_type = (event >> 4);
		const uint8_t channel = event & 15;

		switch (event_type)
		{
		case 0x8:
			{
				const uint8_t note_number = data[offset];
				offset += 2;
				std::cout << "Channel " << uint32_t(channel) << " Note release " << uint32_t(note_number) << std::endl;
			}
			break;

		case 0x9:
			{
				const uint8_t note_number = data[offset];
				std::cout << "Channel " << uint32_t(channel) << " Note press " << uint32_t(note_number) << std::endl;
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
				++offset;
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
			//++offset;
		}
	}

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

SoundData MakeMIDISound(const std::vector<uint8_t>& data)
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

	return LoadTrack(data.data() + sizeof(MIDIHeader), data.size() - sizeof(MIDIHeader));
}
