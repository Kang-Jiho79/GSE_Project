#pragma once

#include "TaintedPieceActor.h"

// A nation's queen, taken back by the god as the queen of its board: a tall
// piece of clean white or black stone with a coronet. Fast and far-reaching,
// as a queen moves: she dashes along any of the eight lines through her
// square, two and three times running, and every third move throws a star
// of shot down all eight at once. While she stands, nothing touches her king.
//
// Fought with her king (KingPieceActor) in the throne room; its floor is
// their board.
class QueenPieceActor : public TaintedPieceActor
{
public:
	// light: the light's queen (white); otherwise the dark's (black).
	// still: set out only to be seen, never to wake (KingPieceActor).
	QueenPieceActor(int region, bool light, bool still = false);

protected:
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void Reward(const ActorContext& ctx) override;
	void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const override;
	void DrawHead(DrawList& out, float depth, float x, float base, float top) const override;
	float Scale() const override;
	int BoardHalf() const override;

private:
	enum class State
	{
		Wait,
		Windup,
		Dash,
		Star,
		Recover,
	};

	enum class Blow
	{
		Dash,
		Star,
	};

	// The one of the eight lines nearest the way to him, and how far along
	// it she can go before the board ends.
	void AimDash(const ActorContext& ctx);
	void Start(Blow blow, float windup);
	void ThrowRing(const ActorContext& ctx);
	void Finish(float cooldown);

	bool m_Light;
	bool m_Still;
	State m_State = State::Wait;
	Blow m_Blow = Blow::Dash;
	float m_StateTime = 0.f;
	float m_Windup = 0.f;
	float m_Cooldown = 1.2f;
	int m_MoveCount = 0;

	int m_DashesLeft = 0;
	float m_DirCol = 1.f;
	float m_DirRow = 0.f;
	float m_Length = 0.f;

	int m_RingsLeft = 0;
	int m_RingIndex = 0;
	float m_NextRing = 0.f;
};
