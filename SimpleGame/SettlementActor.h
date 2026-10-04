#pragma once

#include "Actor.h"
#include "Settlement.h"
#include "Regions.h"

// A village or capital standing in the world: its square, houses, temple,
// altar and notice board - and in a capital, the king's palace and the carpet
// before it - built in its people's way. The light build tall in
// pale stone under pointed roofs, the dark low and heavy in black stone, the
// borderborn in scavenged wood, and where the gray has been there are only
// ruins.
//
// The temples differ outside, but every altar is the same - the same god is
// kept in all of them.
class SettlementActor : public Actor
{
public:
	SettlementActor(const Settlement::Layout& layout, Regions::Style style, bool capital);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void DrawSquare(DrawList& out, const ActorContext& ctx) const;
	void DrawHouse(DrawList& out, float depth, float x, float groundY, const Settlement::Building& building) const;
	void DrawTemple(DrawList& out, float depth, float x, float groundY, const Settlement::Building& building) const;
	void DrawPalace(DrawList& out, float depth, float x, float groundY, const Settlement::Building& building) const;
	void DrawCarpet(DrawList& out, const ActorContext& ctx) const;
	void DrawAltar(DrawList& out, const ActorContext& ctx) const;
	void DrawBoard(DrawList& out, float depth, float x, float groundY) const;

	Settlement::Layout m_Layout;
	Regions::Style m_Style;
	bool m_Capital;
	float m_Time = 0.f;
};
