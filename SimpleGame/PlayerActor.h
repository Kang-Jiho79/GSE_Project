#pragma once

#include "Actor.h"
#include "Alignment.h"

// The one who holds both powers at once. Moves on the screen axes, throws bolts
// at the mouse, and carries the two halves of the vessel on his own two sides.
class PlayerActor : public Actor
{
protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void Move(const ActorContext& ctx);
	void Fire(const ActorContext& ctx, Alignment alignment) const;

	float m_LightCooldown = 0.f;
	float m_DarkCooldown = 0.f;
};
