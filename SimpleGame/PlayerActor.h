#pragma once

#include "Actor.h"
#include "Alignment.h"

// The one who holds both powers at once. Moves on the screen axes, throws bolts
// at the mouse, and carries the two halves of the vessel on his own two sides.
//
// Drawn as a jointed figure - head, torso, two arms, two legs - that walks
// when he moves, breathes when he does not, and looks toward the cursor.
class PlayerActor : public Actor
{
public:
	// Puts him back on his feet at a new spot with nothing left over from
	// before - no roll, combo or slash still in progress.
	void Respawn(float x, float y);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void Move(const ActorContext& ctx);
	void StartDodge(const ActorContext& ctx);
	void Roll(const ActorContext& ctx);
	void StepOnGrid(float dirCol, float dirRow, float tiles);
	void Attack(const ActorContext& ctx);
	void Fire(const ActorContext& ctx, Alignment alignment) const;
	void Slash(const ActorContext& ctx);
	void DrawSlash(DrawList& out, float screenX, float screenY) const;

	float m_AttackCooldown = 0.f;

	// The roll: time left in it, time until the next one, and its direction in
	// tiles, fixed when it starts.
	float m_DodgeTimer = 0.f;
	float m_DodgeCooldown = 0.f;
	float m_DodgeDirCol = 0.f;
	float m_DodgeDirRow = 0.f;

	// Dark form's combo: which cut comes next, and how long he has to make it
	// before it starts over from the first.
	int m_ComboStep = 0;
	float m_ComboTimer = 0.f;

	// The last cut, kept on screen briefly as a trail. Direction is in tiles.
	int m_SlashStep = 0;
	float m_SlashTimer = 0.f;
	float m_SlashDirCol = 0.f;
	float m_SlashDirRow = 0.f;

	// Animation state. The walk phase advances with distance covered rather than
	// time, so the stride always matches the ground going by.
	float m_Time = 0.f;
	float m_WalkPhase = 0.f;
	bool m_Moving = false;
	float m_Facing = 1.f; // +1 looks to the right of the screen, -1 to the left

	// Eased toward the terrace he is standing on, so stepping up a cliff is a
	// climb rather than a jump cut.
	float m_GroundHeight = 0.f;
	bool m_GroundInitialized = false;
};
