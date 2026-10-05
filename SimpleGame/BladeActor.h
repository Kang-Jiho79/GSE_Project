#pragma once

#include "Actor.h"

// A blade of darkness bursting up out of the ground and sinking back - one
// of the many the dark form's planted sword calls up. Something to see only;
// the blow is dealt by whatever raised it.
class BladeActor : public Actor
{
public:
	// delay: seconds before it breaks the surface. lean: pixels its tip
	// leans to the side, so a field of them does not stand in ranks.
	BladeActor(float delay, float lean, float scale);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	float m_Delay;
	float m_Lean;
	float m_Scale;
	float m_Age = 0.f;
	bool m_Burst = false;
};
