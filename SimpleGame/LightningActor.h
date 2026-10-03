#pragma once

#include "Actor.h"

// Called lightning. Marks the ground first, so anything that sees it coming
// can step out, then strikes everything still inside. The player calls it
// down on monsters; an eroded light mage calls it down on the player, its
// mark washed toward ash.
class LightningActor : public Actor
{
public:
	enum class Target
	{
		Monsters,
		Player,
	};

	// Radius in tiles; delay is how long the mark shows before it lands.
	LightningActor(Target target, float radiusTiles, float delay, float damage);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void DrawWarning(DrawList& out, float x, float groundY) const;
	void DrawStrike(DrawList& out, float x, float groundY) const;

	Target m_Target;
	float m_Radius;
	float m_Delay;
	float m_Damage;

	float m_Age = 0.f;
	bool m_Struck = false;

	// Sideways kinks of the bolt, rolled once so it does not flicker.
	static const int kKinks = 5;
	float m_Kinks[kKinks];
};
