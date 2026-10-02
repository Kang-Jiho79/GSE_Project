#pragma once

#include "FormMoves.h"

// Light form: ranged. Safe, steady and low on damage - bolts fired at the
// cursor for as long as the button is held.
class LightFormMoves : public FormMoves
{
public:
	void Update(const ActorContext& ctx, PlayerActor& player) override;
	void Attack(const ActorContext& ctx, PlayerActor& player) override;
	// E: a piercing beam along the aim.
	void Skill(const ActorContext& ctx, PlayerActor& player) override;
	// Q: lightning called down on the cursor, after a warning.
	void Special(const ActorContext& ctx, PlayerActor& player) override;
	// Dark -> light: throws everything nearby back and covers his way out.
	void Enter(const ActorContext& ctx, PlayerActor& player) override;

private:
	float m_BoltCooldown = 0.f;
};
