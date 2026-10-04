#pragma once

#include "Actor.h"
#include "Form.h"
#include "LightFormMoves.h"
#include "DarkFormMoves.h"
#include "AshFormMoves.h"

struct PlayerStatus;

// The one who holds both powers at once. Moves on the screen axes, fights in
// whichever form he is in, and carries the two halves of the vessel on his own
// two sides. What each form does with the attack buttons lives in its own
// FormMoves; this class only moves, rolls, and hands the buttons over.
//
// Drawn as a jointed figure - head, torso, two arms, two legs - that walks
// when he moves, breathes when he does not, and looks toward the cursor.
class PlayerActor : public Actor
{
public:
	// Puts him back on his feet at a new spot with nothing left over from
	// before - no roll, combo or slash still in progress.
	void Respawn(float x, float y);

	// Puts him in a form without the burst a switch sets off - how a king's
	// duel begins. Whatever the old form had under way is dropped.
	void SettleForm(PlayerStatus& status, Form form);

	// Carried along the ground on his own, whatever the keys say - a dash.
	// Direction is a unit vector in tiles. He cannot walk, roll or attack
	// until it ends.
	void Lunge(float dirCol, float dirRow, float tiles, float seconds);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void Move(const ActorContext& ctx);
	void StartDodge(const ActorContext& ctx);
	void Roll(const ActorContext& ctx);
	void LungeStep(const ActorContext& ctx);
	bool IsTravelling() const;
	void StepOnGrid(float dirCol, float dirRow, float tiles);
	FormMoves& MovesFor(Form form);

	LightFormMoves m_LightMoves;
	DarkFormMoves m_DarkMoves;
	AshFormMoves m_AshMoves;

	// The roll: time left in it, time until the next one, and its direction in
	// tiles, fixed when it starts.
	float m_DodgeTimer = 0.f;
	float m_DodgeCooldown = 0.f;
	float m_DodgeDirCol = 0.f;
	float m_DodgeDirRow = 0.f;

	// A lunge in progress: time left, speed in tiles per second, direction.
	float m_LungeTimer = 0.f;
	float m_LungeSpeed = 0.f;
	float m_LungeDirCol = 0.f;
	float m_LungeDirRow = 0.f;

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
