#pragma once

#include "Actor.h"
#include "Alignment.h"

// The one who holds both powers at once. Moves on the screen axes, throws bolts
// at the mouse, and carries the two halves of the vessel on his own two sides.
//
// Drawn as a jointed figure - head, torso, two arms, two legs - that walks
// when he moves, breathes when he does not, and looks toward the cursor.
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

	// Animation state. The walk phase advances with distance covered rather than
	// time, so the stride always matches the ground going by.
	float m_Time = 0.f;
	float m_WalkPhase = 0.f;
	bool m_Moving = false;
	float m_Facing = 1.f; // +1 looks to the right of the screen, -1 to the left

	// Eased toward the terrace he is standing on, so stepping up a cliff is a
	// climb rather than a jump cut.
	float m_GroundHeight = 0.f;
	bool m_GroundInitialized = false;
};
