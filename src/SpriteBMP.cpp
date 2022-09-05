#include "SpriteBMP.hpp"
#include <cassert>
#include <cstdint>


#pragma pack(push,2)
struct SpriteBMP::BitmapFileHeader
{
	uint16_t type;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t off_bits;
} ;
#pragma pack(pop)

#pragma pack(push, 1)
struct SpriteBMP::BitmapInfoHeader
{
	uint32_t size;
	int32_t width;
	int32_t height;
	uint16_t planes;
	uint16_t bit_count;
	uint32_t compression;
	uint32_t size_image;
	int32_t x_pels_per_meter;
	int32_t  y_pels_per_meter;
	uint32_t clr_used;
	uint32_t clr_important;
};
#pragma pack(pop)

SpriteBMP::SpriteBMP(const uint8_t* const file_data)
	: file_data_(file_data)
{
	static_assert(sizeof(SpriteBMP::BitmapFileHeader) == 14, "invalide size");
	static_assert(sizeof(SpriteBMP::BitmapInfoHeader) == 40, "invalide size");

	assert(GetFileHeader().size >= 40);
	assert(GetInfoHeader().bit_count == 8); // Only 8-bit images supported.
}

uint32_t SpriteBMP::GetWidth() const
{
	return uint32_t(GetInfoHeader().width);
}

uint32_t SpriteBMP::GetRowStride() const
{
	return (GetWidth() + 3u) & ~3u;
}

uint32_t SpriteBMP::GetHeight() const
{
	return uint32_t(GetInfoHeader().height);
}

const uint8_t* SpriteBMP::GetImageData() const
{
	return file_data_ + GetFileHeader().off_bits;
}

const Color32* SpriteBMP::GetPalette() const
{
	return reinterpret_cast<const Color32*>(file_data_ + sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader));
}

const SpriteBMP::BitmapFileHeader& SpriteBMP::GetFileHeader() const
{
	return *reinterpret_cast<const BitmapFileHeader*>(file_data_);
}

const SpriteBMP::BitmapInfoHeader& SpriteBMP::GetInfoHeader() const
{
	return *reinterpret_cast<const BitmapInfoHeader*>(file_data_ + sizeof(BitmapFileHeader));
}
