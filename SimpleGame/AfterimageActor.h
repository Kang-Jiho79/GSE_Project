#pragma once

#include "Actor.h"
#include "Alignment.h"

// His outline left behind for a moment where he just was, in the colour of
// a power - what a dash or a flash of movement leaves in its wake. Flat and
// translucent, fading as it goes.
class AfterimageActor : public Actor
{
public:
	// side: -1..1, which way he was turned, so the hood and cloak sit right.
	AfterimageActor(Alignment alignment, float side);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	Alignment m_Alignment;
	float m_Side;
	float m_Age = 0.f;
};
