#pragma once

#include <cstdlib>

// Randomness for actors that actually keep state between frames. Terrain and
// ruins deliberately do not use this - they must regenerate identically every
// frame, so they hash their own grid position instead.
namespace Random
{
	inline float Unit()
	{
		return (rand() % 1000) / 1000.f;
	}

	inline float Angle()
	{
		return (rand() % 6283) / 1000.f;
	}

	inline int Range(int lowInclusive, int highExclusive)
	{
		return lowInclusive + (rand() % (highExclusive - lowInclusive));
	}
}
