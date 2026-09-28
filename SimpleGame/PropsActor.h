#pragma once

#include "Actor.h"

// Everything standing on the ground that is not alive: dead trees, boulders,
// and the broken stonework of whatever was here before. All gray - the world
// keeps no colour of its own.
//
// Like the terrain it holds no state and is regenerated from the grid each
// frame. Trees cluster into groves by a slow noise; stonework keeps to the
// clearings between them.
class PropsActor : public Actor
{
protected:
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void DrawTree(DrawList& out, float depth, float x, float groundY, int variant, float shade) const;
	void DrawBoulder(DrawList& out, float depth, float x, float groundY, int variant, float shade) const;
	void DrawRubble(DrawList& out, float depth, float x, float groundY, int variant, float shade) const;
	void DrawPillar(DrawList& out, float depth, float x, float groundY, int variant, float shade) const;
	void DrawWall(DrawList& out, float depth, float x, float groundY, int variant, float shade) const;
	void DrawTower(DrawList& out, float depth, float x, float groundY, int variant, float shade) const;
	void DrawHouse(DrawList& out, float depth, float x, float groundY, int variant, float shade) const;
};
