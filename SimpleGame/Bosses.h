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

	// Where a region's boss waits in the open - its arena - in tiles. The
	// same spot in every region, kept clear of anything that would wall it in.
	void Arena(int region, float& col, float& row);

	// Whether a region's arena is laid out as a board - wherever one of the
	// god's pieces, whole or tainted, waits - and the tile at its centre.
	// The board runs Tuning::TaintedPiece::kBoardHalf squares each way.
	bool HasBoard(int region);
	void BoardCenter(int region, int& col, int& row);
}
