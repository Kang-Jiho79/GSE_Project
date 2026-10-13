#pragma once

#include "EnemyActor.h"
#include "Alignment.h"

// A core of the god's ward: a shard of its power hanging over one square of
// the board. Nothing cuts it - only the gray's unravelling, run straight
// through it, undoes it. While any of a ward's cores stand, the god cannot
// be touched (GodActor).
//
// The god places them and clears them; broken, one bursts and is left
// empty until then.
class WardCoreActor : public EnemyActor
{
public:
	explicit WardCoreActor(Alignment alignment);

	// Blows glance off it.
	float TakeHit(float damage) override;
	// Run through by a line: undone.
	void Pierce() override;
	float BodyRadius() const override;

	bool IsBroken() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	Alignment m_Alignment;
	float m_Age = 0.f;
	bool m_Broken = false;
	bool m_BurstPending = false;
	float m_Glance = 0.f; // a blow glancing off, fading
};
