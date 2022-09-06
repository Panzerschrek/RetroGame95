#include "Rand.hpp"
#include <cstring>
#include <cassert>

Rand::Rand(const RandResultType seed)
	: generator_(seed)
{}

Rand::RandResultType Rand::Next()
{
	return generator_();
}
