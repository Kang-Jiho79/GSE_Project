#pragma once

#include "Actor.h"

// The terraced ground. One actor for the whole surface rather than one per
// tile: tiles carry no state of their own, so they are regenerated from their
// own grid position every frame and only the visible ones are ever built.
//
// Where one of the god's pieces waits, the ground is laid out as a board:
// squares pale and dark round a centre tile. In the god's own place the
// board is all there is - its squares in full, and nothing past them;
// elsewhere they are only a pattern worked into the region's own ground.
class TerrainActor : public Actor
{
public:
	// from/to: squares from the centre on both axes. whole: the god's place.
	void SetBoard(int centerCol, int centerRow, int from, int to, bool whole);

protected:
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	bool m_Board = false;
	bool m_Whole = false;
	int m_BoardCol = 0;
	int m_BoardRow = 0;
	int m_From = 0;
	int m_To = 0;
};
