#pragma once

#include "FormMoves.h"

// Dark form: melee. Risky, heavy, and the only way to heal in a fight - a
// three-cut combo whose last cut hits hardest and throws out a shockwave, a
// dash that leaves his shadow behind, and the sword driven into the ground
// to raise blades under everything near, with a share of every blow that
// lands coming back as health.
class DarkFormMoves : public FormMoves
{
public:
	void Update(const ActorContext& ctx, PlayerActor& player) override;
	void Attack(const ActorContext& ctx, PlayerActor& player) override;
	// E: a dash along the aim, cutting everything in the way.
	void Skill(const ActorContext& ctx, PlayerActor& player) override;
	// Q: the sword into the ground - blades of darkness burst up under every
	// enemy round him. He stands still while the sword is down.
	void Special(const ActorContext& ctx, PlayerActor& player) override;
	// Light -> dark: drags everything nearby in and hurts it on arrival.
	void Enter(const ActorContext& ctx, PlayerActor& player) override;
	void Interrupt() override;
	void Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const override;

	// The sword is in the ground: he neither moves nor swings.
	bool IsPlanting() const;

private:
	void DrawCut(DrawList& out, const PlayerActor& player, float screenX, float groundY) const;
	void DrawSmash(DrawList& out, const PlayerActor& player, float screenX, float groundY) const;

	float m_Cooldown = 0.f;

	// Which cut comes next, and how long he has to make it before the combo
	// starts over from the first.
	int m_ComboStep = 0;
	float m_ComboTimer = 0.f;

	// The last cut, kept on screen briefly as a trail - or, for the last of
	// the three, where it came down. Direction is in tiles.
	int m_SlashStep = 0;
	float m_SlashTimer = 0.f;
	float m_SlashDirCol = 0.f;
	float m_SlashDirRow = 0.f;
	float m_SmashX = 0.f; // world point the last cut came down on
	float m_SmashY = 0.f;
	float m_SmashRadius = 0.f;

	// The sword in the ground: time left before he pulls it out.
	float m_PlantTimer = 0.f;
};
