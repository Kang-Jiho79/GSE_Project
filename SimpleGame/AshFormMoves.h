#pragma once

#include "FormMoves.h"

class EnemyActor;

// Ash form (잿빛폼): the gray Kinis left him, taken up at the very end - and
// with it light and dark given up, so this is all he has left. A scythe that
// cuts, dashes and cuts again: a four-cut combo whose last cut bursts a second
// time along its trail, a dash to the far side of whoever is ahead, a parry
// that answers a blow from behind its owner, a line that unravels shields
// and wards alike, and a burst that wipes every shot out of the air.
//
// Every number is fixed (Tuning::AshForm), and nothing it does feeds on or
// tilts his balance; the gray is its own thing.
class AshFormMoves : public FormMoves
{
public:
	// Cooldowns, and whatever was left to go off later: the last cut's
	// trail, the blast where a dash set off, the answer to a parried blow.
	void Update(const ActorContext& ctx, PlayerActor& player) override;
	void Attack(const ActorContext& ctx, PlayerActor& player) override;
	// E: a line along the aim that unravels whatever it passes through.
	void Skill(const ActorContext& ctx, PlayerActor& player) override;
	// Q: everything round him wiped away - the shots in the air, and a heavy
	// blow to all within reach - and a moment untouched.
	void Special(const ActorContext& ctx, PlayerActor& player) override;
	// Never switched into: the gray is the only form left once he holds it.
	void Enter(const ActorContext& ctx, PlayerActor& player) override;
	void Interrupt() override;
	void Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const override;

	// Right click, in place of a dodge. Returns whether he went.
	bool DashCut(const ActorContext& ctx, PlayerActor& player);
	// Space, in place of a switch.
	void Parry(const ActorContext& ctx, PlayerActor& player);

private:
	// A blow he parried, answered.
	void Counter(const ActorContext& ctx, PlayerActor& player);
	// The nearest enemy within reach - only inside the wedge round the
	// direction, unless the spread is a full turn. nullptr for none.
	EnemyActor* Nearest(const ActorContext& ctx, const PlayerActor& player, float reachTiles,
		float dirCol, float dirRow, float spreadDegrees) const;
	// Puts him on the far side of an enemy at once - or as far as a wall
	// lets him. Gives the direction he went, in tiles.
	void GetBehind(PlayerActor& player, const EnemyActor& enemy, float& dirCol, float& dirRow) const;

	float m_Cooldown = 0.f;
	float m_DashCooldown = 0.f;

	// Which cut comes next, and how long he has to make it before the combo
	// starts over.
	int m_ComboStep = 0;
	float m_ComboTimer = 0.f;

	// The last cut, kept on screen as it sweeps and fades. Direction in tiles.
	int m_CutStep = 0;
	float m_CutTimer = 0.f;
	float m_CutDirCol = 1.f;
	float m_CutDirRow = 0.f;

	// The last cut's trail, waiting to burst, and then bursting: where it
	// was cut from and which way.
	float m_EchoDelay = 0.f;
	float m_EchoTimer = 0.f;
	float m_EchoX = 0.f;
	float m_EchoY = 0.f;
	float m_EchoDirCol = 1.f;
	float m_EchoDirRow = 0.f;

	// The image he left where a dash set off, waiting to burst.
	float m_BlastDelay = 0.f;
	float m_BlastX = 0.f;
	float m_BlastY = 0.f;

	// What is left of the parry's stance, for its ring.
	float m_StanceTime = 0.f;
};
