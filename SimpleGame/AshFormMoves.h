#pragma once

#include "FormMoves.h"

// Ash form (잿빛폼): the gray Kinis left him, taken up at the very end. Both
// powers at once, neither the one nor the other - so it fights between them,
// reaching past a blade and short of a bolt, and what it does best is undo:
// a line that unravels shields and wards alike, and a burst that wipes every
// shot out of the air.
//
// Nothing it does feeds on or tilts his balance; the gray is its own thing.
class AshFormMoves : public FormMoves
{
public:
	void Update(const ActorContext& ctx, PlayerActor& player) override;
	// A wave of the gray across a wedge of ground in front of him, wiping
	// the shots it passes; every third in a row goes all the way round.
	void Attack(const ActorContext& ctx, PlayerActor& player) override;
	// E: a line along the aim that unravels whatever it passes through.
	void Skill(const ActorContext& ctx, PlayerActor& player) override;
	// Q: everything round him wiped away - the shots in the air, and a heavy
	// blow to all within reach - and a moment untouched.
	void Special(const ActorContext& ctx, PlayerActor& player) override;
	// Into ash: the shots nearest him wiped away, a blow to all that close,
	// and a moment untouched.
	void Enter(const ActorContext& ctx, PlayerActor& player) override;
	void Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const override;

private:
	float m_Cooldown = 0.f;
	// Waves in a row, and how long until the row is broken.
	int m_WaveCount = 0;
	float m_ChainTimer = 0.f;

	// The last wave, kept on screen as it spreads. Direction is in tiles.
	float m_WaveTimer = 0.f;
	float m_WaveDirCol = 1.f;
	float m_WaveDirRow = 0.f;
	bool m_WaveRing = false;
};
