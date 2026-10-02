#pragma once

#include "Actor.h"
#include "Alignment.h"

// A ring of one power flashing across the ground and gone - the swap skills'
// burst. Purely something to see; the hit itself has already been dealt.
class BurstActor : public Actor
{
public:
	// inward: the ring closes on its centre instead of spreading from it,
	// for a burst that pulls rather than pushes.
	BurstActor(Alignment alignment, float radiusTiles, bool inward);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	Alignment m_Alignment;
	float m_Radius;
	bool m_Inward;
	float m_Age = 0.f;
};
