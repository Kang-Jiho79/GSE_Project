#pragma once

#include "Regions.h"
#include "Story.h"
#include <string>

// What the world remembers of his journey, apart from the man himself: where
// he has been, where he will rise if he falls, and what he has overcome.
// Kept in one place so it can all be saved together.
struct Progress
{
	bool visited[Regions::Count] = {};

	// The last temple he rested at; until he finds one, the hidden village's
	// waystone.
	int respawnRegion = Regions::Limina;
	bool respawnAtAltar = false;

	// A region's boss, once beaten, is gone for good.
	bool bossDefeated[Regions::Count] = {};

	// The kings who have acknowledged him, each by losing a duel to him.
	// Both together give him a name.
	bool recognizedByLucius = false;
	bool recognizedByNoctis = false;

	bool HasTitle() const
	{
		return recognizedByLucius && recognizedByNoctis;
	}

	// What he is called; what he typed when he began.
	std::string heroName = "이름 없는 자";

	bool story[Story::Count] = {};

	bool Has(Story::Flag flag) const
	{
		return flag != Story::NoFlag && story[flag];
	}

	void Set(Story::Flag flag)
	{
		if (flag != Story::NoFlag)
		{
			story[flag] = true;
		}
	}
};
