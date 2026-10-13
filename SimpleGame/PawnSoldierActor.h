#pragma once

#include "EnemyActor.h"

// One of the tainted pawn's soldiers: a small black pawn stepping toward the
// far rank beside it, a square at a time, taking only on the forward
// diagonals. Stand square in front of one and it cannot step.
//
// It does not see to its own end: fallen, it scatters what it was and waits
// to be cleared; at the far rank it waits to give itself to its pawn. The
// pawn (TaintedPawnActor) looks after both.
class PawnSoldierActor : public EnemyActor
{
public:
	// On square col,row; it marches toward endRow, a square of row at a time.
	PawnSoldierActor(int col, int row, int endRow);

	float TakeHit(float damage) override;
	float BodyRadius() const override;

	bool IsDead() const;
	bool ReachedEnd() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Wait,
		Step,
		Windup,
	};

	void PlaceOn(float col, float row);

	int m_Col;
	int m_Row;
	int m_EndRow;
	float m_Health;
	bool m_Dead = false;
	bool m_MotesPending = false;

	State m_State = State::Wait;
	float m_StateTime = 0.f;
	float m_Cooldown;
	float m_HitFlash = 0.f;
	float m_Time = 0.f;
};
