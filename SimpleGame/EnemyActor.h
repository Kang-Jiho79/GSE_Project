#pragma once

#include "Actor.h"

// Anything his attacks can land on - the eroded, and the bosses. Combat
// finds its targets through this, so a new kind of foe only has to say how
// it takes a blow.
class EnemyActor : public Actor
{
public:
	// Returns how much of it actually landed - never more than it had left, so
	// overkill does not count toward anything that feeds on damage dealt.
	virtual float TakeHit(float damage) = 0;
	// A blow a shield could catch, coming from the given world point. Without
	// a shield it lands like any other.
	virtual float TakeHit(float damage, float fromX, float fromY);

	// Run straight through by a line blow - a beam, a dash - just before its
	// damage lands. Nothing to most; a ward that only such a blow can break,
	// breaks.
	virtual void Pierce();

	// Knocked along the ground by this many tiles. The heaviest stand firm.
	virtual void Shove(float offsetCol, float offsetRow);

	// How far its body reaches from its centre, in tiles, so a big one is
	// struck from further off.
	virtual float BodyRadius() const;
};
