#pragma once

#include "Actor.h"

// The terraced ground. One actor for the whole surface rather than one per
// tile: tiles carry no state of their own, so they are regenerated from their
// own grid position every frame and only the visible ones are ever built.
//
// In the god's place the ground is its board instead: squares pale and dark
// round a centre tile (Tuning::God::kBoardFrom..kBoardTo), and nothing past
// them.
class TerrainActor : public Actor
{
public:
	TerrainActor();
	TerrainActor(int boardCol, int boardRow);

protected:
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	bool m_Board = false;
	int m_BoardCol = 0;
	int m_BoardRow = 0;
};
