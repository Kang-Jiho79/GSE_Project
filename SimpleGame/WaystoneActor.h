#pragma once

#include "Actor.h"

// The standing stone every region has where its roads arrive. He comes in
// beside it, and standing at it he can open the world map and set out again.
class WaystoneActor : public Actor
{
protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	float m_Time = 0.f;
};
