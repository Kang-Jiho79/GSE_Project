#pragma once

#include <vector>
#include "Actor.h"

// The terraced ground. One actor for the whole surface rather than one per
// tile: tiles carry no state of their own, so they are regenerated from their
// own grid position every frame and only the visible ones are ever built.
//
// Where one of the god's pieces waits, the ground is laid out as a board:
// squares pale and dark round a centre tile. In the god's own place the
// board is all there is - its squares in full, and nothing past them;
// elsewhere they are only a pattern worked into the region's own ground.
//
// Between the places of the world there is no ground at all, only the fog
// the window clears to (WorldGrid::ZoneAt).
class TerrainActor : public Actor
{
public:
	// A rectangle of squares, from and to squares out from the centre along
	// each axis; boards sharing a centre share its colouring. whole: the
	// god's place, where nothing lies past the board.
	void AddBoard(int centerCol, int centerRow, int fromCol, int toCol, int fromRow, int toRow, bool whole);

protected:
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	struct Board
	{
		int centerCol;
		int centerRow;
		int fromCol;
		int toCol;
		int fromRow;
		int toRow;
	};

	std::vector<Board> m_Boards;
	bool m_Whole = false;
};
