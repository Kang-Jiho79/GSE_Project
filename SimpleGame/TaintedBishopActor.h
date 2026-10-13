#pragma once

#include "TaintedPieceActor.h"

// The tainted bishop (잠식된 비숍): a white bishop, the boss of the border. It
// moves only on the diagonals - flashing along one to the square that puts
// him on its line - and strikes along all four of them at once: from a pale
// square, beams of light; from a dark one, slow orbs of dark rolling out
// along them. A true bishop never leaves its colour; this one, eaten by the
// gray, now and then takes one straight step and changes it.
//
// Past half its life it strikes the board itself: every diagonal of one
// direction but every third, all at once. The safe ground is a lattice.
class TaintedBishopActor : public TaintedPieceActor
{
public:
	explicit TaintedBishopActor(int region);

	const char* Hint() const override;

protected:
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const override;
	void DrawHead(DrawList& out, float depth, float x, float base, float top) const override;

private:
	enum class State
	{
		Wait,
		Windup,
		Recover,
	};

	enum class Blow
	{
		Blink,   // along a diagonal, to the square that puts him on its line
		Beam,    // light along all four diagonals
		Orb,     // dark rolling out along all four
		Shift,   // one straight step, onto the other colour
		Lattice, // all but every third diagonal of one direction
	};

	void Choose(const ActorContext& ctx);
	void Release(const ActorContext& ctx);
	void Square(int& col, int& row) const;
	bool OnPale() const;
	// Whether a square is struck by the lattice under way.
	bool IsLatticeSquare(int col, int row) const;
	float WindupTime() const;

	State m_State = State::Wait;
	Blow m_Blow = Blow::Blink;
	float m_StateTime = 0.f;
	float m_Cooldown = 1.f;
	int m_MoveCount = 0;
	int m_LatticeCount = 0;

	// Where a blink or a step will put it, in squares.
	int m_ToCol = 0;
	int m_ToRow = 0;
	// The lattice: which direction of diagonal, and which of every three is
	// spared.
	bool m_LatticeRising = false;
	int m_LatticeSafe = 0;
};
