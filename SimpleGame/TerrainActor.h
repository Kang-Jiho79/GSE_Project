#pragma once

#include "Actor.h"

// The terraced ground. One actor for the whole surface rather than one per
// tile: tiles carry no state of their own, so they are regenerated from their
// own grid position every frame and only the visible ones are ever built.
class TerrainActor : public Actor
{
protected:
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;
};
