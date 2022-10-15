#pragma once
#include "Color.hpp"
#include <cstdint>

// Simple wrapper for BMP static data.
// Only 8-bit (indexed) images are supported.
class SpriteBMP
{
public:
	constexpr SpriteBMP(const uint8_t* file_data) : file_data_(file_data) {}

	uint32_t GetWidth() const;
	uint32_t GetRowStride() const;
	uint32_t GetHeight() const;

	const uint8_t* GetImageData() const;
	const Color32* GetPalette() const;

private:
	struct BitmapFileHeader;
	struct BitmapInfoHeader;

private:
	const BitmapFileHeader& GetFileHeader() const;
	const BitmapInfoHeader& GetInfoHeader() const;

private:
	const uint8_t* file_data_ = nullptr;
};
