#pragma once

#include "Actor.h"
#include "Alignment.h"

// A mote of the colour a monster turned out to have been. It bursts outward,
// settles, and is drawn in once the player gets close enough - no button, just
// proximity.
class ParticleActor : public Actor
{
public:
	ParticleActor(Alignment alignment, float velX, float velY);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	Alignment m_Alignment;
	float m_VelX;
	float m_VelY;
	float m_Life;
};
