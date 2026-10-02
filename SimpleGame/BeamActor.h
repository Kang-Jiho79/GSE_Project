#pragma once

#include "Actor.h"
#include "Alignment.h"

// A straight streak of one power along the ground, seen for a moment - the
// light form's piercing beam, or the trail of the dark form's dash. The hit is
// dealt the instant it is cast; this only shows where it went.
class BeamActor : public Actor
{
public:
	// Direction is a unit vector on the ground; length in tiles.
	BeamActor(Alignment alignment, float dirCol, float dirRow, float lengthTiles);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	Alignment m_Alignment;
	float m_DirCol;
	float m_DirRow;
	float m_Length;
	float m_Age = 0.f;
};
