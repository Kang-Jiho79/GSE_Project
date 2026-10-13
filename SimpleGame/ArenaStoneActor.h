#pragma once

#include "Actor.h"

// Stone standing on a tainted rook's board, one square across: one of the
// four pillars that stand there from the start, or a length of the walls the
// rook raises. Either blocks his way and shots while it stands.
//
// A charging rook breaks what it runs into (TaintedRookActor): a pillar is
// left as rubble - and the rook stunned - while a wall crumbles away. A wall
// crumbles on its own, too, after a while. The rook clears away crumbled
// walls; pillars stay, and stand again if the fight starts over.
class ArenaStoneActor : public Actor
{
public:
	// lifetime: how long a wall stands before it crumbles; a pillar's is
	// ignored.
	ArenaStoneActor(int col, int row, bool pillar, float lifetime);

	void Break();
	// A broken pillar stands again.
	void Restore();
	bool IsPillar() const;
	bool IsBroken() const;
	// A wall that has crumbled away entirely, ready to be cleared.
	bool IsGone() const;
	int Col() const;
	int Row() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void Block(bool block);

	int m_Col;
	int m_Row;
	bool m_Pillar;
	float m_Lifetime;
	float m_Age = 0.f;
	bool m_Blocking = false;
	bool m_Broken = false;
	bool m_DustPending = false;
	float m_BrokenTime = 0.f;
};
