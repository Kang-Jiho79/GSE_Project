#pragma once

#include "TaintedPieceActor.h"

// A nation's king, taken back by the god as the king of its board: the
// tallest piece, crowned with a cross, in clean white or black stone. Slow,
// as a king moves - one square at a time - but every blow of his is heavy:
// he slams every square round him, and calls check, setting the row and the
// column through where he stands burning, to drive him into a corner.
//
// He wears his nation's ward - the light's king turns aside light, the
// dark's dark - so only the other power hurts him. And while his queen
// stands, nothing does: she must fall first. When he falls, it is
// checkmate; the scene takes it from there.
//
// still: set out only to be seen, never to wake - the king that closes in
// at the very end, when one nation's pair was left standing.
class KingPieceActor : public TaintedPieceActor
{
public:
	KingPieceActor(int region, bool light, bool still = false);

	const char* Hint() const override;

protected:
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void Reward(const ActorContext& ctx) override;
	float Resist(float damage) override;
	void OnFall(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;
	void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const override;
	void DrawHead(DrawList& out, float depth, float x, float base, float top) const override;
	float Scale() const override;
	int BoardHalf() const override;

private:
	enum class State
	{
		Wait,
		Step,
		Windup,
		Recover,
	};

	enum class Blow
	{
		Slam,
		Check,
	};

	void Choose(const ActorContext& ctx, float distance);
	void Start(Blow blow, float windup);
	void Release(const ActorContext& ctx);
	void UpdateCheck(const ActorContext& ctx);
	void Finish();
	// Whether a point lies in the row or the column of the check.
	bool InCheck(float col, float row) const;
	bool Shields(Form form) const;

	bool m_Light;
	bool m_Still;
	// Whether his queen still stands, as of the last frame.
	bool m_Guarded = true;

	State m_State = State::Wait;
	Blow m_Blow = Blow::Slam;
	float m_StateTime = 0.f;
	float m_Windup = 0.f;
	float m_Cooldown = 1.5f;
	int m_MoveCount = 0;

	// The square he is stepping from and to.
	float m_FromCol = 0.f;
	float m_FromRow = 0.f;
	float m_ToCol = 0.f;
	float m_ToRow = 0.f;

	// The check: through which square, and how long it has left to burn.
	int m_CheckCol = 0;
	int m_CheckRow = 0;
	float m_CheckLeft = 0.f;
	float m_CheckTick = 0.f;
	float m_SlamFlash = 0.f;
};
