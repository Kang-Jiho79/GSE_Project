#pragma once

#include "TaintedPieceActor.h"

class ArenaStoneActor;

// The tainted rook (잠식된 룩): a rook of both colours, the boss of the front.
// It charges straight along its board's lines - never on a diagonal - until
// it runs into something, and raises lengths of wall behind him to pen him
// in for the next charge.
//
// Four pillars stand on its board. Charging into one breaks it and leaves the
// rook stunned for a while, taking heavier blows - so the way to fight it is
// to stand where a pillar lies between them. Its own walls only crumble when
// it runs into them.
class TaintedRookActor : public TaintedPieceActor
{
public:
	explicit TaintedRookActor(int region);

	const char* Hint() const override;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void OnFall(const ActorContext& ctx) override;
	float Resist(float damage) override;
	void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const override;
	void DrawHead(DrawList& out, float depth, float x, float base, float top) const override;

private:
	enum class State
	{
		Wait,
		Windup,
		Charge,
		Stunned,
		Recover,
	};

	enum class Blow
	{
		Charge,
		Wall,
	};

	static const int kPillars = 4;
	static const int kMostWalls = 15;

	void RaisePillars(const ActorContext& ctx);
	void ClearWalls();
	void Choose(const ActorContext& ctx);
	void RaiseWall(const ActorContext& ctx);
	void UpdateCharge(const ActorContext& ctx);
	// Along whichever of the board's two axes he lies further, toward him.
	void AxisToward(const ActorContext& ctx, int& dirCol, int& dirRow) const;
	int PillarsLeft() const;

	State m_State = State::Wait;
	Blow m_Blow = Blow::Charge;
	float m_StateTime = 0.f;
	float m_Cooldown = 1.f;
	int m_MoveCount = 0;

	// The charge under way: from where, which way, how far, and whether it
	// has struck him.
	float m_FromCol = 0.f;
	float m_FromRow = 0.f;
	int m_DirCol = 1;
	int m_DirRow = 0;
	float m_Length = 0.f;
	bool m_Landed = false;

	// The wall it is about to raise: its middle square, and which way it runs.
	int m_WallCol = 0;
	int m_WallRow = 0;
	int m_WallAlongCol = 0;
	int m_WallAlongRow = 1;

	// The board's stone: owned by the graph; only this clears the walls.
	ArenaStoneActor* m_Pillar[kPillars] = {};
	ArenaStoneActor* m_Wall[kMostWalls] = {};
	bool m_PillarsRaised = false;
};
