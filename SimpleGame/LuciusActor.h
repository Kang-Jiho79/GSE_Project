#pragma once

#include "KingActor.h"

// Lucius, the light's king (빛의 왕 루키우스). Fought in light form alone, so
// the fight is one of range: he keeps his distance and circles, throws fans
// of spears, and calls judgment down along the ground through where he stood.
// Press him close and a ring of light throws back at him before he steps away.
//
// His goodness is the twisted kind - between blows he shields and heals
// himself under a sanctuary that turns every blow aside. Only a beam, going
// straight through, breaks it, and leaves him reeling and open for a while.
class LuciusActor : public KingActor
{
public:
	LuciusActor(int region, bool recognized);

	float BodyRadius() const override;
	void Pierce() override;

protected:
	void Fight(const ActorContext& ctx) override;
	void ResetMoves() override;
	void Reward(const ActorContext& ctx) override;
	float Resist(float damage) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Walk,
		Windup,
		Volley,    // throwing wave after wave
		Sanctuary, // shielded and healing
		Stagger,   // the sanctuary broken under him
		Recover,
	};

	enum class Blow
	{
		Volley,
		Judgment,
		Radiance,
	};

	void Walk(const ActorContext& ctx, float distance, float dirCol, float dirRow);
	void Choose(const ActorContext& ctx, float distance);
	void Start(Blow blow, float windup);
	void Release(const ActorContext& ctx, float dirCol, float dirRow);
	void ThrowWave(const ActorContext& ctx, float dirCol, float dirRow);
	void Finish(float cooldown);

	// Judgment falls along both axes, and once he is worn down along both
	// diagonals as well.
	int JudgmentLines() const;
	void JudgmentLine(int index, float& dirCol, float& dirRow) const;

	void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const;
	void DrawJudgment(DrawList& out, const ActorContext& ctx, float fill, float alpha, bool burning) const;
	void DrawSanctuary(DrawList& out, float depth, float x, float groundY) const;
	void DrawBody(DrawList& out, float depth, float x, float groundY, float worldX, float worldY) const;

	State m_State = State::Walk;
	Blow m_Blow = Blow::Volley;
	float m_StateTime = 0.f;
	float m_Windup = 0.f;
	float m_Cooldown = 1.f;
	float m_SanctuaryCooldown = 0.f;
	int m_BlowCount = 0;

	int m_WavesLeft = 0;
	int m_WaveIndex = 0;
	float m_NextWave = 0.f;

	// Where judgment will fall, fixed when he begins to call it.
	float m_TargetCol = 0.f;
	float m_TargetRow = 0.f;

	// Which way he circles, and until he turns.
	float m_StrafeSign = 1.f;
	float m_StrafeTimer = 0.f;

	float m_JudgmentFlash = 0.f; // the pillars, just after they fall
	float m_StrikeFlash = 0.f;
	bool m_BreakPending = false;

	bool m_Moving = false;
	float m_Gait = 0.f;
};
