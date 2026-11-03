#pragma once

#include "Regions.h"
#include "Story.h"
#include "Quests.h"
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

	// The kings who have acknowledged him, each by losing a duel to him -
	// and each giving him a name of its own (Titles).
	bool recognizedByLucius = false;
	bool recognizedByNoctis = false;

	// Both kings: half of what the true ending asks.
	bool HasKingRecognition() const
	{
		return recognizedByLucius && recognizedByNoctis;
	}

	// Both nations' king and queen of the board broken: the last of what
	// the true ending asks.
	bool HasBrokenBothPairs() const
	{
		return story[Story::LightPairBroken] && story[Story::DarkPairBroken];
	}

	// What he is called; what he typed when he began.
	std::string heroName = "이름 없는 자";

	bool story[Story::Count] = {};

	// Each notice-board request: where it stands (a Quests::Stage), and how
	// many of the gray a hunt has counted.
	int questStage[Quests::Count] = {};
	int questCount[Quests::Count] = {};

	// Whether the border village's keeper has already told him how the
	// powers work, so the second time he says something else.
	bool metEveningKeeper = false;

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
