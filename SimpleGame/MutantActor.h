#pragma once

#include "BossActor.h"

// A gray mutant (잿빛 변이체): one of the light or the dark, swallowed so
// completely by the gray that it lost its mind and its shape. A hunched mass
// on many arms, what it was still showing in the cracks across it.
//
// It slams the ground where he stands, sweeps everything in front of it, and
// every third blow spits the gray out in a ring. Past half its health it rages
// and calls eroded soldiers to it.
class MutantActor : public BossActor
{
public:
	enum class Origin
	{
		Light,
		Dark,
		Mixed, // of both, which the borderland's mutant is
	};

	MutantActor(int region, Origin origin, float maxHealth);

	float BodyRadius() const override;

protected:
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void Reward(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Stalk,
		Windup,
		Recover,
	};

	enum class Blow
	{
		Slam,
		Sweep,
		Burst,
	};

	bool Raging() const;
	float WindupTime(Blow blow) const;
	void StartBlow(const ActorContext& ctx, Blow blow, float dirCol, float dirRow);
	void Release(const ActorContext& ctx);
	void SummonGuards(const ActorContext& ctx);
	// The colour of one of its cracks, or of one shot of its burst.
	Alignment ColorOf(int index) const;

	void DrawTelegraph(DrawList& out, float x, float groundY) const;

	Origin m_Origin;
	State m_State = State::Stalk;
	Blow m_Blow = Blow::Slam;
	float m_StateTime = 0.f;
	float m_Cooldown = 1.f;
	int m_BlowCount = 0;
	bool m_Summoned = false;

	// Fixed when a blow starts, so it can be dodged: the way it faces, and
	// for a slam, the ground it will strike.
	float m_DirCol = 1.f;
	float m_DirRow = 0.f;
	float m_TargetCol = 0.f;
	float m_TargetRow = 0.f;

	float m_StrikeFlash = 0.f;
};
