#pragma once

#include "Settlement.h"

class SceneGraph;
struct Progress;

// Who lives in each settlement, where they stand, and what they talk about.
// A temple of the light or the dark keeps a priest beside it; the ruined
// ones keep none. Once the gray has found the hidden village, no one is left
// there.
namespace Npcs
{
	void Spawn(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph);
}
