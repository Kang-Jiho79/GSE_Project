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
	// until it ends. trail: his shadow is left behind along the way.
	void Lunge(float dirCol, float dirRow, float tiles, float seconds, bool trail = true);

	// What his body does as he fights - called by his forms the moment a
	// move goes off, so he throws his hand out, swings, or raises it.
	enum class Action
	{
		None,
		Cast,     // a hand thrust out, and the recoil
		Beam,     // both hands out, pushed back by it
		Call,     // a hand raised to the sky
		Slash,    // a quick cut, from one side across to the other
		Sweep,    // a wide cut back the other way, the whole body turning
		Finisher, // up and down, with a hop
		Thrust,   // the blade or the hand straight out
		Swap,     // arms flung wide as the form turns over
	};
	// aimX/aimY: toward where, on screen - he turns to it at once. Zero
	// keeps the way he faces.
	void Act(Action action, float aimX, float aimY);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void Move(const ActorContext& ctx);
	void StartDodge(const ActorContext& ctx);
	// The light's dodge: a flash straight to a spot a little way off.
	void Blink(const ActorContext& ctx);
	// The dark's: a dash through as a shadow.
	void ShadowDash(const ActorContext& ctx);
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
	// Until the next shadow left behind during a dash, and whether this one
	// leaves any.
	float m_AfterimageTimer = 0.f;
	bool m_LungeTrail = true;

	// The move his body is making, how far through it he is, toward where on
	// screen, and which side the next cut swings from.
	Action m_Action = Action::None;
	float m_ActionTimer = 0.f;
	float m_ActionLength = 1.f;
	float m_ActionX = 1.f;
	float m_ActionY = 0.f;
	float m_SlashSide = 1.f;

	// Animation state. The walk phase advances with distance covered rather than
	// time, so the stride always matches the ground going by.
	float m_Time = 0.f;
	float m_WalkPhase = 0.f;
	bool m_Moving = false;
	float m_Facing = 1.f; // +1 looks to the right of the screen, -1 to the left
	// Which way he looks on screen, in radians - 0 to the right, a quarter
	// turn up into the screen - turned smoothly toward the cursor rather
	// than snapped. He starts facing out of the screen.
	float m_Heading = -1.5707963f;
	// The way he last walked, on screen, for his cloak to trail against.
	float m_StrideX = 0.f;
	float m_StrideY = 0.f;

	// Eased toward the terrace he is standing on, so stepping up a cliff is a
	// climb rather than a jump cut.
	float m_GroundHeight = 0.f;
	bool m_GroundInitialized = false;
};
