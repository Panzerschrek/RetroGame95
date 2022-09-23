#include "Rand.hpp"
#include <cstring>
#include <cassert>

Rand::Rand(const RandResultType seed)
	: generator_(seed)
{}

Rand Rand::CreateWithRandomSeed()
{
	return Rand(std::random_device()());
}

Rand::RandResultType Rand::Next()
{
	return generator_();
}

float Rand::RandomAngle()
{
	return float(Next()) * (2.0f * 3.1415926535f / float(c_max_rand_plus_one_));
}
