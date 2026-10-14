#pragma once

#include "Actor.h"

// The ground a boss of the god's pieces holds (Bosses::HasBoard): its board
// walled round, so it reads at a glance as no ordinary field. A great chess
// piece of stone stands behind it, seen from far off; an approach lined with
// smaller ones leads to the one opening, flanked by two tall pillars - the
// threshold - and the walls and fog keep everything else out.
//
// While its boss is awake and he is inside, the opening is sealed: there is
// no walking away from the fight (Scene sees to when).
class ArenaActor : public Actor
{
public:
	// Lays out its walls - and the blockers that go with them - round the
	// region's board.
	explicit ArenaActor(int region);

	void SetSealed(bool sealed);
	// Whether a point is inside its walls.
	bool IsInside(float col, float row) const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	// The opening, as a box in tiles.
	void OpeningBox(float& minCol, float& minRow, float& maxCol, float& maxRow) const;
	void DrawStatue(DrawList& out, const ActorContext& ctx, float col, float row, float scale, bool white, int kind) const;

	int m_CenterCol;
	int m_CenterRow;
	int m_DirCol;
	int m_DirRow;
	bool m_Sealed = false;
	float m_Time = 0.f;
};
