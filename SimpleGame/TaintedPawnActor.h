#pragma once

#include "TaintedPieceActor.h"
#include "Tuning.h"

class PawnSoldierActor;

// The tainted pawn (잠식된 폰): a black pawn, the boss of the woods outside
// the hidden village. It starts on its board's back rank with two soldiers
// beside it and steps toward the far rank a square at a time. It takes only
// on the forward diagonals - so square in front of it is safe, and standing
// there, he stops it stepping at all.
//
// At the far rank it is promoted, and mends; it is promoted anyway once half
// its life is gone. Promoted it stands taller and slides along the eight
// lines like a queen. A soldier that reaches the far rank gives itself to
// it, mending it a little.
class TaintedPawnActor : public TaintedPieceActor
{
public:
	explicit TaintedPawnActor(int region);

	const char* Hint() const override;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void OnFall(const ActorContext& ctx) override;
	void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const override;
	void DrawHead(DrawList& out, float depth, float x, float base, float top) const override;
	float Scale() const override;

private:
	enum class State
	{
		Wait,
		Step,    // onto the next square
		Windup,  // gathering a diagonal strike or, promoted, a slide
		Slide,
	};

	void Promote(const ActorContext& ctx, bool reachedEnd);
	void SpawnSoldiers(const ActorContext& ctx);
	void ClearSoldiers();
	void LookAfterSoldiers();
	// Where it stands, in squares.
	void Square(int& col, int& row) const;
	float Windup() const;

	State m_State = State::Wait;
	float m_StateTime = 0.f;
	float m_Cooldown = 1.f;
	bool m_Promoted = false;
	bool m_Sliding = false; // the windup under way is for a slide

	// The move under way: from where to where, and whether it has struck.
	float m_FromCol = 0.f;
	float m_FromRow = 0.f;
	float m_ToCol = 0.f;
	float m_ToRow = 0.f;
	bool m_Landed = false;

	// Its soldiers: owned by the graph, cleared only by it.
	PawnSoldierActor* m_Soldiers[Tuning::TaintedPawn::kSoldiers] = {};
	bool m_SoldiersOut = false;
};
