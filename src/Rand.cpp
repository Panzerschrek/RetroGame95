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
