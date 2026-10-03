#pragma once

#include "Actor.h"
#include "Alignment.h"

// A spell thrown at the player by one of the eroded - a mage's bolt or orb.
// It flies along the ground, optionally bending toward him, and spends itself
// on the first touch that lands. A roll carries him through it unharmed.
class EnemyProjectileActor : public Actor
{
public:
	// Direction is a unit vector on the ground; speed in tiles per second.
	// turnRate is how fast it can bend toward the player, in radians per
	// second - 0 flies straight.
	EnemyProjectileActor(Alignment alignment, float dirCol, float dirRow,
		float speed, float turnRate, float lifetime, float damage);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void BendToward(float targetCol, float targetRow, float deltaTime);

	Alignment m_Alignment;
	float m_DirCol;
	float m_DirRow;
	float m_Speed;
	float m_TurnRate;
	float m_Lifetime;
	float m_Damage;
	float m_Age = 0.f;
};
