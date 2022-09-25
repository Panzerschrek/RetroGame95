#include "MIDI.hpp"
#include <optional>
#include <cassert>
#include <cstring>

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

struct TrackHeader
{
	char type[4];
	uint32_t length;
};
#pragma pack(pop)

static_assert(sizeof(MIDIHeader) == 14, "Invalid size");
static_assert(sizeof(TrackHeader) == 8, "Invalid size");

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
	return uint16_t(
		((x & 0x00FF) << 8) |
		((x & 0xFF00) >> 8));
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

	const float freq = base_freq * std::pow(mul, float(*note - base_freq_note));

	const uint32_t shift = 16;
	const uint32_t freq_scaled = uint32_t(float(1 << shift) * freq / float(sample_rate));
	for(size_t i = sound_data.samples.size() - duration; i < sound_data.samples.size(); ++i)
	{
		sound_data.samples[i] = (((i * freq_scaled) >> shift) & 1) == 0 ? (-127) : (127);
	}
}

SoundData LoadTrack(
	const uint8_t* data, const size_t data_size,
	const uint32_t sample_rate,
	const float time_scaler)
{
	const auto header = reinterpret_cast<const TrackHeader*>(data);

	const uint32_t length = ByteSwap(header->length);

	assert(std::strncmp(header->type, "MTrk", 4) == 0);
	assert(length <= data_size);

	SoundData result;

	Channels channels;

	float tempo = 0.5f;

	size_t offset = sizeof(TrackHeader);
	bool first_event = true;
	while(offset < std::min(size_t(length), data_size))
	{
		const uint32_t delta_time = ReadVarLen(data, offset);
		const uint8_t event = data[offset];
		++offset;

		const uint32_t delta_samples = uint32_t(float(delta_time * sample_rate) * tempo * time_scaler);
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
				(void) note_number;
				offset += 2;
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
				channel.num_presses += 1;
				channel.note_number = std::max(channel.note_number, uint32_t(note_number));
				offset += 2;
			}
			break;

		case 0xA:
			// Polyphonic Key Pressure.
			offset += 2;
			break;

		case 0xB:
			// Control change.
			offset += 2;
			break;

		case 0xC:
			// Program change.
			++offset;
			break;

		case 0xD:
			// Channel pressure.
			++offset;
			break;

		case 0xE:
			// Pitch wheel change.
			offset += 2;
			break;

		case 0xF:
			if(event == 0xFF)
			{
				const uint8_t meta_event = data[offset];
				++offset;

				const uint32_t meta_length = ReadVarLen(data, offset);

				if(meta_event == 0x51)
				{
					tempo = float((data[offset] << 16) | (data[offset + 1] << 8) | (data[offset + 2] << 0)) / 1000000.0f;
				}

				offset += meta_length;
			}
			else
			{
				offset += ReadVarLen(data, offset);
			}
			break;

		default:
			// Unrecognized event.
			break;
		}
		if(event < 0x80)
		{
			// Some spezialized event.
			offset += 1;
		}
	}

	return result;
}

} // namespace

SoundData MakeMIDISound(const uint8_t* const data, size_t const data_size, const uint32_t sample_rate)
{
	const auto header = reinterpret_cast<const MIDIHeader*>(data);
	assert(std::strncmp(header->type, "MThd", 4) == 0);
	assert(ByteSwap(header->length) == 6);
	assert(ByteSwap(header->format) <= 1);

	const auto division = int16_t(ByteSwap(uint16_t(header->division)));

	const float time_scaler = 1.0f / float(division);

	return LoadTrack(data + sizeof(MIDIHeader), data_size - sizeof(MIDIHeader), sample_rate, time_scaler);
}
