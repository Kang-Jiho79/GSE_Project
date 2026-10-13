#pragma once

#include "Actor.h"

// The pale mark - the same one that hangs over every altar - shown where the
// god reaches into the world: over a king who was asked the wrong question,
// and on the god's own face. It swells in, turns slowly about its upright
// axis, and stays until whatever showed it is gone.
//
// Placed at a point on the ground; it hangs above it.
class GodMarkActor : public Actor
{
public:
	// lift: pixels above the ground. size: the mark's height when fully
	// grown, in pixels.
	GodMarkActor(float lift, float size);

	// The mark itself, for anything else that bears it. spin is the turn
	// about its upright axis, in radians.
	static void DrawMark(DrawList& out, float depth, float x, float y, float size, float spin, float alpha);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	float m_Lift;
	float m_Size;
	float m_Age = 0.f;
};
