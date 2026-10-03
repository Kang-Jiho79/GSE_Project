#pragma once

#include "Actor.h"

// A dark mage's curse laid on the ground under the player. It takes a moment
// to form - enough to see it and step off - then bites anyone standing in it
// every tick until it fades.
class CurseFieldActor : public Actor
{
protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	float m_Age = 0.f;
	float m_TickTimer = 0.f;
};
