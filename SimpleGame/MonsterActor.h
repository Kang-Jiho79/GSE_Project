#pragma once

#include "Actor.h"
#include "Alignment.h"

// One of the eroded: a soldier of the light or the dark that the gray has
// taken. It wanders its own patch until the player comes near, then closes
// and fights the way its side always did. What it was is still in how it
// stands and moves, and is what the player absorbs when it dies.
//
// Every blow is gathered first, with the ground it will cover marked, so the
// player can read it and step or roll out.
class MonsterActor : public Actor
{
public:
	enum class Kind
	{
		Knight, // closes in and fights hand to hand
		Mage,   // keeps its distance and casts
	};

	MonsterActor(Kind kind, Alignment alignment);

	// Returns how much of it actually landed - never more than it had left, so
	// overkill does not count toward anything that feeds on damage dealt.
	float TakeHit(float damage);
	// A blow a shield can catch, coming from the given world point.
	float TakeHit(float damage, float fromX, float fromY);

	// Knocked along the ground by this many tiles. It is carried there over a
	// moment rather than teleported, and any blow it was gathering is lost.
	void Shove(float offsetCol, float offsetRow);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Roam,
		Chase,
		Windup,   // gathering a blow; the ground it will cover is marked
		Charge,   // a dark knight's rush along a marked line
		Recover,  // the moment after a blow, open to a counter
	};

	enum class Blow
	{
		Cleave,    // light knight: one frontal cut
		TwinCut,   // dark knight: two cuts in quick succession
		Charge,    // dark knight: a rush along a line
		Bolt,      // light mage: a straight bolt
		Smite,     // light mage: lightning marked under the player
		Orb,       // dark mage: an orb that bends after the player
		Curse,     // dark mage: a cursed patch under the player
	};

	void UpdateChase(const ActorContext& ctx, float distance, float dirCol, float dirRow);
	void KeepDistance(const ActorContext& ctx, float distance, float dirCol, float dirRow);
	void Strafe(const ActorContext& ctx, float dirCol, float dirRow);
	bool TryStartKnightBlow(float distance, float dirCol, float dirRow);
	bool TryStartMageBlow(float distance, float dirCol, float dirRow);
	bool TryStartBlow(float distance, float dirCol, float dirRow);
	void StartWindup(Blow blow, float windup, float dirCol, float dirRow);
	void Release(const ActorContext& ctx);
	void UpdateCharge(const ActorContext& ctx);
	void UpdateRecover(const ActorContext& ctx);
	void Cut(const ActorContext& ctx);
	void Cast(const ActorContext& ctx);

	void Roam(const ActorContext& ctx);
	void StepToward(float targetCol, float targetRow, float speed, float deltaTime);
	void StepAlong(float dirCol, float dirRow, float tiles);
	void FaceAlong(float dirCol, float dirRow);
	void SpawnDeathBurst(const ActorContext& ctx) const;

	void DrawTelegraph(DrawList& out, float x, float groundY) const;

	Kind m_Kind;
	Alignment m_Alignment;
	State m_State = State::Roam;
	float m_StateTime = 0.f; // seconds since the current state began

	// The blow being gathered or thrown, and the direction on the ground it
	// was aimed in when it started - fixed, so it can be dodged.
	Blow m_Blow = Blow::Cleave;
	float m_BlowWindup = 0.f;
	float m_BlowDirCol = 1.f;
	float m_BlowDirRow = 0.f;
	bool m_SecondCutPending = false;
	bool m_ChargeLanded = false;

	// The everyday blow, and the bigger one with its own longer wait - a
	// dark knight's charge, a mage's lightning or curse.
	float m_AttackCooldown = 0.f;
	float m_SpecialCooldown = 0.f;

	// A dark mage's sidestep: which way, and until it turns back.
	float m_StrafeSign = 1.f;
	float m_StrafeTimer = 0.f;

	float m_StrikeFlash = 0.f; // 1 as a blow lands, fading

	// Struck: a white flash fading over a moment, and sparks thrown off on
	// the next update, where it can reach the scene to spawn them.
	float m_HitFlash = 0.f;
	bool m_SparksPending = false;

	float m_Health;

	// Where it lives and wanders, in tiles.
	float m_OriginCol = 0.f;
	float m_OriginRow = 0.f;
	float m_RoamTargetCol = 0.f;
	float m_RoamTargetRow = 0.f;
	float m_RoamWaitTimer = 0.f;
	bool m_Anchored = false;

	// The way it faces on the ground, for its shield; and on screen, for
	// drawing.
	float m_FaceCol = 1.f;
	float m_FaceRow = 0.f;
	float m_Facing = 1.f;

	// A shove in progress: its speed in world pixels per second, and how long
	// it has left.
	float m_ShoveVelX = 0.f;
	float m_ShoveVelY = 0.f;
	float m_ShoveTimer = 0.f;

	float m_Time = 0.f;
	float m_GaitPhase = 0.f; // advances with distance covered
	bool m_Moving = false;

	// Eased toward the terrace it stands on, so it climbs cliffs instead of
	// jumping between levels.
	float m_GroundHeight = 0.f;
};
