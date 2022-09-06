#pragma once
#include <random>

class Rand
{
public:
	using RandResultType = uint32_t;

	explicit Rand(RandResultType seed= 0);

	RandResultType Next();

private:
	// Simple and fast generator.
	// Produces good result for bits 0-31.
	// Parameters, same as in rand() from glibc.
	using Generator= std::linear_congruential_engine< RandResultType, 1103515245u, 12345u, 1u << 31u > ;
	using ExtendedType= uint64_t;

public:
	static constexpr RandResultType c_max_rand_plus_one_= Generator::modulus;

private:
	Generator generator_;
};
