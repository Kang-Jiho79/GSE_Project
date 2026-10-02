#pragma once

#include "Actor.h"

class PlayerActor;

// What one form does with the attack buttons. The player owns one of each and
// hands the buttons to whichever matches his current form. Both keep ticking,
// so a cooldown started in one form still runs out while he is in the other.
class FormMoves
{
public:
	virtual ~FormMoves() {}

	// Every frame, in either form - cooldowns run down, and anything that
	// keeps going after the button (a spin) carries on.
	virtual void Update(const ActorContext& ctx, PlayerActor& player) = 0;
	virtual void Attack(const ActorContext& ctx, PlayerActor& player) = 0;

	// E and Q. Each checks and starts its own cooldown.
	virtual void Skill(const ActorContext& ctx, PlayerActor& player) {}
	virtual void Special(const ActorContext& ctx, PlayerActor& player) {}

	// The swap skill: fired the moment he switches into this form, so the
	// switch itself is a move in the fight.
	virtual void Enter(const ActorContext& ctx, PlayerActor& player) = 0;

	// Drops anything left half-done, such as a combo, when he leaves the form.
	virtual void Interrupt() {}

	// Effects drawn around him. groundY is where his feet are on screen.
	virtual void Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const {}
};
