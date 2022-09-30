#pragma once
#include <cstddef>
#include <cstdint>

uint32_t ExtractUTF8CodePoint(const char*& ptr);

// Returns length in code points.
size_t UTF8StringLen(const char* str);

// Replacement for "snprintf". Do not use it, because it is too heavy.
void NumToString(char* str, size_t str_len, uint32_t num, uint32_t target_num_digits);
