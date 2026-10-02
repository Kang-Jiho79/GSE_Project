#pragma once

#include "Actor.h"
#include "Alignment.h"

// A gray thing that lost its colour and its mind with it. It wanders its own
// patch until the player comes near, then closes and hits. What it used to be
// only shows when it dies.
//
// Comes in three builds that move differently. None of them hints at the
// alignment - that would give away the one thing a live monster must hide.
class MonsterActor : public Actor
{
public:
	enum class Kind
	{
		Brute,   // wide and heavy; plods, hits hard to look at
		Stalker, // tall and thin; sways, quick
		Crawler, // low slab on many small legs; skitters
	};

	MonsterActor(Alignment alignment, Kind kind);

	// Returns how much of it actually landed - never more than it had left, so
	// overkill does not count toward anything that feeds on damage dealt.
	float TakeHit(float damage);

	// Knocked along the ground by this many tiles. It is carried there over a
	// moment rather than teleported, and is staggered until it lands.
	void Shove(float offsetCol, float offsetRow);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Roam,
		Chase,
		Attack,
	};

	void UpdateState(float distanceToPlayer);
	void MoveToward(float targetX, float targetY, float speed, float deltaTime);
	void Roam(const ActorContext& ctx);
	void SpawnDeathBurst(const ActorContext& ctx) const;

	void DrawBrute(DrawList& out, float depth, float x, float groundY, float lunge) const;
	void DrawStalker(DrawList& out, float depth, float x, float groundY, float lunge) const;
	void DrawCrawler(DrawList& out, float depth, float x, float groundY, float lunge) const;
	void DrawEyes(DrawList& out, float depth, float x, float y, float spacing) const;

	Alignment m_Alignment;
	Kind m_Kind;
	State m_State = State::Roam;

	float m_OriginX = 0.f;
	float m_OriginY = 0.f;
	float m_RoamTargetX = 0.f;
	float m_RoamTargetY = 0.f;
	float m_RoamWaitTimer = 0.f;

	float m_AttackCooldown = 0.f;
	float m_Health;

	// A shove in progress: its speed in world pixels per second, and how long
	// it has left.
	float m_ShoveVelX = 0.f;
	float m_ShoveVelY = 0.f;
	float m_ShoveTimer = 0.f;
	bool m_Anchored = false;

	float m_Time = 0.f;
	float m_GaitPhase = 0.f; // advances with distance covered
	float m_Facing = 1.f;

	// Eased toward the terrace it stands on, so it climbs cliffs instead of
	// jumping between levels.
	float m_GroundHeight = 0.f;
};
