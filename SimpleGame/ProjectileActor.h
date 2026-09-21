#pragma once

#include "Actor.h"
#include "Alignment.h"

// A bolt of one power or the other, thrown at wherever the mouse was pointing.
// It finds what it hit by asking the scene graph for monsters rather than being
// handed a list of them.
class ProjectileActor : public Actor
{
public:
	ProjectileActor(Alignment alignment, float velX, float velY);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	Alignment m_Alignment;
	float m_VelX;
	float m_VelY;
	float m_Life;
};
