#include "String.hpp"
#include <algorithm>

uint32_t ExtractUTF8CodePoint(const char*& ptr)
{
	// c_bit_masks[4] - menas save first 4 bits
	static const uint32_t c_bit_masks[9]=
	{
		(1 << 0) - 1,
		(1 << 1) - 1,
		(1 << 2) - 1,
		(1 << 3) - 1,
		(1 << 4) - 1,
		(1 << 5) - 1,
		(1 << 6) - 1,
		(1 << 7) - 1,
		(1 << 8) - 1,
	};

	const char c= *ptr;
	uint32_t code = 0;

	if( ( c & 0b10000000 ) == 0 )
	{
		code = uint32_t(c);
		++ptr;
	}
	else if( ( c & 0b11100000 ) == 0b11000000 )
	{
		code=
			( (uint32_t(ptr[0]) & c_bit_masks[5]) << 6u ) |
			( (uint32_t(ptr[1]) & c_bit_masks[6]) << 0u );

		ptr += 2;
	}
	else if( ( c & 0b11110000 ) == 0b11100000 )
	{
		code=
			( (uint32_t(ptr[0]) & c_bit_masks[4]) << 12u ) |
			( (uint32_t(ptr[1]) & c_bit_masks[6]) <<  6u ) |
			( (uint32_t(ptr[2]) & c_bit_masks[6]) <<  0u );

		ptr += 3;
	}
	else if( ( c & 0b11111000 ) == 0b11110000 )
	{
		code=
			( (uint32_t(ptr[0]) & c_bit_masks[3]) << 18u ) |
			( (uint32_t(ptr[1]) & c_bit_masks[6]) << 12u ) |
			( (uint32_t(ptr[2]) & c_bit_masks[6]) <<  6u ) |
			( (uint32_t(ptr[3]) & c_bit_masks[6]) <<  0u );

		ptr += 4;
	}
	else
	{
		// Codes above unicode range - wtf?
		++ptr;
	}

	return code;
}

size_t UTF8StringLen(const char* str)
{
	size_t length = 0;
	while(ExtractUTF8CodePoint(str) != 0)
	{
		++length;
	}

	return length;
}

void NumToString(char* const str, const size_t str_len, uint32_t num, const uint32_t target_num_digits)
{
	if(str_len == 0)
	{
		return;
	}
	if(str_len == 1)
	{
		str[0] = '\0';
		return;
	}


	char* dst = str;
	char* const dst_end = str + (str_len - 1);
	uint32_t num_digits = 0;
	do
	{
		const uint32_t digit = num % 10;
		num /= 10;
		*dst = char('0' + digit);
		++dst;
		++num_digits;
	} while(num != 0 && dst < dst_end);

	while(num_digits < target_num_digits && dst < dst_end)
	{
		*dst = ' ';
		++dst;
		++num_digits;
	}

	for(uint32_t i = 0; i < num_digits / 2; ++i)
	{
		std::swap(str[i], str[num_digits - i - 1]);
	}

	*dst = '\0';
}
