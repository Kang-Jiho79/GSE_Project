#pragma once

#include "Settlement.h"

class SceneGraph;
struct Progress;

// Who lives in each settlement, where they stand, and what they talk about.
// A temple of the light or the dark keeps a priest beside it; the ruined
// ones keep none. Once the gray has found the hidden village, no one is left
// there. The squares with no name have their few people too, and things left
// written there to read.
namespace Npcs
{
	void Spawn(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph);

	// What a notice-board request under way has set out in a place, set out
	// with its monsters as he nears it and cleared with them: a lost lantern,
	// or a peddler and the gray hemming him in.
	void SpawnErrands(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph);
}
