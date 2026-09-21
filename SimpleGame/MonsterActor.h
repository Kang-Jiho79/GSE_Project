#pragma once

#include "Actor.h"
#include "Alignment.h"

// A gray thing that lost its colour and its mind with it. It wanders its own
// patch until the player comes near, then closes and hits. What it used to be
// only shows when it dies.
class MonsterActor : public Actor
{
public:
	explicit MonsterActor(Alignment alignment);

	void TakeHit(float damage);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Roam,
		Chase,
		Attack,
	};

	void UpdateState(float distanceToPlayer);
	void Roam(const ActorContext& ctx);
	void SpawnDeathBurst(const ActorContext& ctx) const;

	Alignment m_Alignment;
	State m_State = State::Roam;

	float m_OriginX = 0.f;
	float m_OriginY = 0.f;
	float m_RoamTargetX = 0.f;
	float m_RoamTargetY = 0.f;
	float m_RoamWaitTimer = 0.f;

	float m_AttackCooldown = 0.f;
	float m_Health;
	bool m_Anchored = false;
};
