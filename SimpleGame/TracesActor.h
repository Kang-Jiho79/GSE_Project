#pragma once

#include "Actor.h"

// What is left of light and dark on the ground, drawn: only what is in view,
// nearest the middle of the screen first, and no more than so many to a
// frame - so many of them alight - however thickly they lie.
//
// Holds no state but the time its lights flicker and its motes drift by;
// what lies where is worked out by Traces each frame.
class TracesActor : public Actor
{
protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void DrawCrack(DrawList& out, float depth, float x, float y, int variant, float r, float g, float b) const;
	void DrawLamp(DrawList& out, float depth, float x, float y, int variant) const;
	void DrawHaloStatue(DrawList& out, float depth, float x, float y, int variant) const;
	void DrawBanner(DrawList& out, float depth, float x, float y, int variant) const;
	void DrawMotes(DrawList& out, float depth, float x, float y, int variant, bool light) const;
	void DrawThorns(DrawList& out, float depth, float x, float y, int variant) const;
	void DrawEmbers(DrawList& out, float depth, float x, float y, int variant) const;
	void DrawCrystal(DrawList& out, float depth, float x, float y, int variant) const;
	void DrawWreck(DrawList& out, float depth, float x, float y, int variant) const;

	float m_Time = 0.f;
};
