#pragma once

#include "FormMoves.h"

// Dark form: melee. Risky, heavy, and the only way to heal in a fight - a
// three-cut combo whose last cut hits hardest, a dash, and a spin, with a
// share of every blow that lands coming back as health.
class DarkFormMoves : public FormMoves
{
public:
	void Update(const ActorContext& ctx, PlayerActor& player) override;
	void Attack(const ActorContext& ctx, PlayerActor& player) override;
	// E: a dash along the aim, cutting everything in the way.
	void Skill(const ActorContext& ctx, PlayerActor& player) override;
	// Q: a spin that keeps cutting all around him for a while, on double
	// lifesteal. He can still walk; he cannot swing or dash until it ends.
	void Special(const ActorContext& ctx, PlayerActor& player) override;
	// Light -> dark: drags everything nearby in and hurts it on arrival.
	void Enter(const ActorContext& ctx, PlayerActor& player) override;
	void Interrupt() override;
	void Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const override;

	bool IsSpinning() const;

private:
	void DrawSpin(DrawList& out, const PlayerActor& player, float screenX, float groundY) const;

	float m_Cooldown = 0.f;

	// Which cut comes next, and how long he has to make it before the combo
	// starts over from the first.
	int m_ComboStep = 0;
	float m_ComboTimer = 0.f;

	// The last cut, kept on screen briefly as a trail. Direction is in tiles.
	int m_SlashStep = 0;
	float m_SlashTimer = 0.f;
	float m_SlashDirCol = 0.f;
	float m_SlashDirRow = 0.f;

	// The spin: time left in it, and until its next cut.
	float m_SpinTimer = 0.f;
	float m_SpinTickTimer = 0.f;
};
