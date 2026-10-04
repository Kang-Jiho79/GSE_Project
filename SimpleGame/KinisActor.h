#pragma once

#include "BossActor.h"

// Kinis (키니스), the first of the gray, at its heart. Untouchable while they
// talk; once the talking is over, he fights with all three of what he is - a
// lance of light thrown along the ground, a cleave of darkness when he is
// pressed close, and every third blow a ring of the gray that both became.
// Worn down, he is quicker and throws three lances at once.
//
// He does not vanish when beaten. He falls to his knees and stays there, for
// his last words; the story decides what happens to him after.
class KinisActor : public BossActor
{
public:
	using BossActor::TakeHit;

	explicit KinisActor(int region);

	float BodyRadius() const override;
	// Nothing reaches him before the fight begins.
	float TakeHit(float damage) override;

	// The talking is over: he fights.
	void Provoke();
	bool HasFallen() const;

protected:
	// Stands waiting until provoked; after falling, only kneels.
	void OnUpdate(const ActorContext& ctx) override;
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	// What he leaves is what he tells, not power to absorb.
	void Reward(const ActorContext& ctx) override;
	void Defeated(const ActorContext& ctx) override;
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
		Lance,
		Cleave,
		Ring,
	};

	bool Worn() const;
	int LanceCount() const;
	float LanceAngle(int index) const;
	void Start(Blow blow, float windup, float dirCol, float dirRow);
	void Release(const ActorContext& ctx);

	void DrawTelegraph(DrawList& out, float x, float groundY) const;
	void DrawBody(DrawList& out, float depth, float x, float groundY) const;

	bool m_Provoked = false;
	bool m_Fallen = false;
	float m_FallenTime = 0.f;
	float m_Breath = 0.f; // his own clock, running even before the fight

	State m_State = State::Stalk;
	Blow m_Blow = Blow::Lance;
	float m_StateTime = 0.f;
	float m_Windup = 0.f;
	float m_Cooldown = 1.f;
	int m_BlowCount = 0;
	float m_DirCol = 1.f;
	float m_DirRow = 0.f;
	float m_StrikeFlash = 0.f;
	bool m_Moving = false;
};
