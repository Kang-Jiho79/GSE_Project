#pragma once

#include "Actor.h"

class KingActor;

// One of Noctis's snares. Plain to see while it is being laid, then all but
// invisible - only a faint glint now and then gives it away. Step on it and it
// springs a moment later. Gone when the duel that laid it ends, or after a
// while on its own.
class SnareActor : public Actor
{
public:
	SnareActor(const KingActor* owner, float damage);

	const KingActor* Owner() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	// The king outlives every snare it lays: they all go with the region.
	const KingActor* m_Owner;
	float m_Damage;
	float m_Age = 0.f;
	float m_GlintPhase = 0.f;

	bool m_Sprung = false;
	bool m_Struck = false;
	float m_SprungTime = 0.f;
};
