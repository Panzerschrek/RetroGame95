#pragma once
#include "Fixed.hpp"

struct Matrix3
{
public:
	// Creates identity matrix.
	Matrix3();
	Matrix3(const fixed16vec3_t& x, const fixed16vec3_t& y, const fixed16vec3_t& z);

	fixed16_t GetDeterminant() const;
	Matrix3 GetTranspose() const;
	Matrix3 GetInverse() const;

public:
	fixed16vec3_t x;
	fixed16vec3_t y;
	fixed16vec3_t z;
};
