#pragma once

#include "Settlement.h"

class SceneGraph;
class BossActor;
struct Progress;

// Which regions keep a boss, and where it waits. Spawned when the region is
// entered, unless he has already beaten it - except a capital's king, who is
// always there at the end of its palace's carpet, and only stops offering a
// duel once it has yielded to him.
namespace Bosses
{
	// Returns the boss spawned, or nullptr if there is none here, or none left.
	BossActor* Spawn(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph);
}
