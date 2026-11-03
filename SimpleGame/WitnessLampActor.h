#pragma once

#include "EnemyActor.h"

class KingActor;

// One of Lucius's witnesses: a lamp of light hung in a corner of the throne
// room, a ring under it running down the time he has to put it out. Shot
// enough, it goes out; left lit, it bears witness when judgment comes. Gone
// once its time is up, or when the duel that lit it ends.
class WitnessLampActor : public EnemyActor
{
public:
	WitnessLampActor(const KingActor* owner, float lifetime);

	float TakeHit(float damage) override;
	float BodyRadius() const override;

	bool IsLit() const;
	// Put out at once, without being shot - judgment has come.
	void Snuff();

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	// The king outlives every lamp it lights: they all go with the region.
	const KingActor* m_Owner;
	float m_Lifetime;
	float m_Health;
	float m_Age = 0.f;
	bool m_Lit = true;
	bool m_SparksPending = false;
	float m_OutTime = 0.f;
};
