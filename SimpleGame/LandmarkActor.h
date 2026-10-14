#pragma once

#include "Actor.h"
#include "Regions.h"

// What stands in a square with no name of its own (Regions::Feature), so
// each is worth walking into: a camp's fire and tents, a great king of stone
// lying on its side, the wreckage of both sides' weapons and banners, pools
// of the gray lying on the ground, the broken pillars round a fallen temple,
// the rocks the borderborn hid among, a resting place where both sides sit
// apart. The houses of a hamlet, and the fallen temple itself, are its
// settlement's (SettlementActor).
class LandmarkActor : public Actor
{
public:
	// Set at the place's landmark spot, blocking the way where it stands.
	LandmarkActor(int region, Regions::Feature feature);

	// Where in a place its landmark stands.
	static void Spot(int region, float& col, float& row);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	// A screen point for a tile offset from the landmark, with its ground,
	// and its depth; false if it is off screen.
	bool Place(const ActorContext& ctx, float dc, float dr, float& x, float& y, float& depth) const;

	void DrawCamp(DrawList& out, const ActorContext& ctx) const;
	void DrawFallenKing(DrawList& out, const ActorContext& ctx) const;
	void DrawWreckage(DrawList& out, const ActorContext& ctx) const;
	void DrawRift(DrawList& out, const ActorContext& ctx) const;
	void DrawRuins(DrawList& out, const ActorContext& ctx) const;
	void DrawHideout(DrawList& out, const ActorContext& ctx) const;
	void DrawPilgrimRest(DrawList& out, const ActorContext& ctx) const;

	Regions::Feature m_Feature;
	float m_Col;
	float m_Row;
	float m_Time = 0.f;
};
