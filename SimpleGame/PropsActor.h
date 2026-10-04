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
public:
	// density scales how much grows and stands here, 1 being an ordinary
	// field. Nothing is placed within clearRadius tiles of the clearing, so a
	// waystone or a village is never buried in trees.
	PropsActor(float density, float clearCol, float clearRow, float clearRadius);

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

	float m_Density;
	float m_ClearCol;
	float m_ClearRow;
	float m_ClearRadius;
};
