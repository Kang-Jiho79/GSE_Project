#pragma once

#include "EnemyActor.h"
#include "Alignment.h"
#include "Form.h"

// What every boss shares. It lies dormant in its arena until he comes near
// (or strikes it), then fights; if he runs far enough it gives up, returns to
// its place and recovers, so a fight cannot be won by attrition from the edge
// of the map. Beaten, it scatters a great deal of power for him to absorb and
// is remembered as gone for good.
//
// What it does while awake, how it looks and what it leaves behind are each
// boss's own.
class BossActor : public EnemyActor
{
public:
	using EnemyActor::TakeHit;

	// region: where it lives, so its defeat is recorded against that place.
	// wakeRange: how near he must come to wake it; negative, and nothing but
	// a blow or RequestWake does.
	// furyTime: how long it can be fought before it starts to rage.
	BossActor(const char* name, float maxHealth, int region, float wakeRange, float leashRange, float furyTime);

	float TakeHit(float damage) override;

	const char* Name() const;
	float HealthFraction() const;
	bool IsAwake() const;
	// 0 until its time runs out, then one higher every interval.
	int FuryLevel() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;

	// The fight itself, every frame while it is awake.
	virtual void Fight(const ActorContext& ctx) = 0;
	// It gave up and went back to its place: forget whatever was under way.
	virtual void OnReset(const ActorContext& ctx);
	// Scatter what he takes from it as it falls.
	virtual void Reward(const ActorContext& ctx) = 0;
	// What a blow actually does to it; a warded boss turns some aside, and a
	// cunning one may take note of the blow to answer it.
	virtual float Resist(float damage);
	// Every frame once its health is gone. By default it gives up its reward,
	// is remembered as beaten, and is gone at once; one that does not die of
	// it has its own ending.
	virtual void Defeated(const ActorContext& ctx);

	// Wakes it next frame, as a blow would.
	void RequestWake();
	// Back to its place, whole and asleep, its fury forgotten.
	void ReturnHome(const ActorContext& ctx);
	// Closes its wounds, never past its full health.
	void Mend(float amount);

	// Throws motes of one power about where it stands, for him to gather.
	void ScatterMotes(const ActorContext& ctx, Alignment alignment, int count) const;
	// Distance to him in tiles, and the way to him on the ground.
	float ToPlayer(const ActorContext& ctx, float& dirCol, float& dirRow) const;
	void StepAlong(float dirCol, float dirRow, float tiles);
	// Faces along a ground direction, as seen on screen.
	void FaceAlong(float dirCol, float dirRow);

	// What fury does to it: everything it does is this much faster, and every
	// blow lands this much harder.
	float FurySpeed() const;
	float FuryDamage() const;
	// A burning glow at its feet, stronger with every level of fury.
	void DrawFury(DrawList& out, float depth, float x, float groundY, float radius) const;

	// The form he was in as of this frame - which power his blows carry.
	Form PlayerForm() const;

	float Facing() const;
	float Time() const;
	float HitFlash() const;
	bool IsDormant() const;

private:
	const char* m_Name;
	float m_MaxHealth;
	float m_Health;
	int m_Region;
	float m_WakeRange;
	float m_LeashRange;
	float m_FuryTime;
	float m_AwakeTime = 0.f;
	int m_FuryLevel = 0;
	Form m_PlayerForm = Form::Light;
	bool m_DeflectPending = false;

	bool m_Anchored = false;
	float m_HomeX = 0.f;
	float m_HomeY = 0.f;

	bool m_Awake = false;
	bool m_WakeRequested = false;

	float m_Facing = 1.f;
	float m_Time = 0.f;
	float m_HitFlash = 0.f;
	bool m_SparksPending = false;
};
