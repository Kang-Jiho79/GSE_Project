#pragma once

#include "Actor.h"

// Where the dark form's sword went into the ground: a circle marked out
// round it, and a moment later a blade of darkness bursting up under every
// enemy inside - and a few more where there is no one, so the field reads
// as a field. Each enemy struck takes the blow, and a share comes back to
// him as health, as from any dark blow.
//
// With more than one wave, it does it all again a moment after.
class BladeFieldActor : public Actor
{
public:
	BladeFieldActor(float radiusTiles, float damage, int waves);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void Erupt(const ActorContext& ctx);

	float m_Radius;
	float m_Damage;
	int m_WavesLeft;
	float m_Age = 0.f;
	float m_NextWave;
};
