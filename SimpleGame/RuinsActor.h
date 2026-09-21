#pragma once

#include "Actor.h"

// Broken stonework scattered over the ground. Like the terrain it holds no
// state, so it is regenerated from the grid each frame rather than stored.
class RuinsActor : public Actor
{
protected:
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;
};
