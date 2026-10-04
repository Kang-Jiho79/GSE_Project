#pragma once

#include "EnemyActor.h"

class KingActor;

// One of Noctis's shades: his shadow, risen out of the ground beside the
// player to come at him and cut once. Struck, it comes apart at once - and
// since there was nothing in it, nothing is drunk from it. Gone when the duel
// that raised it ends.
class ShadeActor : public EnemyActor
{
public:
	ShadeActor(const KingActor* owner, float damage);

	float TakeHit(float damage) override;
	float BodyRadius() const override;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Rise,
		Stalk,
		Windup,
		Fade,
	};

	void Enter(State state);

	// The king outlives every shade it raises: they all go with the region.
	const KingActor* m_Owner;
	float m_Damage;

	State m_State = State::Rise;
	float m_StateTime = 0.f;
	float m_Age = 0.f;
	bool m_Dispersed = false; // struck apart rather than spent
	bool m_DustPending = false;

	float m_DirCol = 1.f;
	float m_DirRow = 0.f;
	float m_Facing = 1.f;
	float m_Gait = 0.f;
};
