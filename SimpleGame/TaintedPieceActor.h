#pragma once

#include "BossActor.h"

// A tainted piece (잠식된 신의 말): a piece from the god's board that the gray
// has eaten into, waiting at the far end of a field region on a board worked
// into the ground. What every one shares: a white or black piece - or one of
// both - gone gray and warped, cracks spreading through it as it is hurt with
// what it was showing through; its board's squares; and its fall, toppling
// onto the board and rolling before it breaks apart and leaves what it was
// for him to absorb.
//
// How it moves and strikes is each piece's own (TaintedPawnActor,
// TaintedBishopActor, TaintedRookActor), as is its head.
class TaintedPieceActor : public BossActor
{
public:
	enum class Origin
	{
		Light, // a white piece
		Dark,  // a black piece
		Mixed, // of both
	};

	TaintedPieceActor(const char* name, int region, Origin origin, float maxHealth, float furyTime);

	float BodyRadius() const override;

protected:
	void Reward(const ActorContext& ctx) override;
	// Topples and rolls, and only then gives up its reward and is gone.
	void Defeated(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

	// The moment it is beaten, before it falls: anything it brought with it
	// can be cleared away.
	virtual void OnFall(const ActorContext& ctx);
	// The ground it is about to strike, under everything else.
	virtual void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const;
	// Its head, from the top of its collar. Drawn with Part, so it falls with
	// the rest.
	virtual void DrawHead(DrawList& out, float depth, float x, float base, float top) const = 0;
	// How big it stands - a promoted pawn grows.
	virtual float Scale() const;
	// How far off the ground it is drawn, mid-leap or mid-charge.
	virtual float Lift() const;

	// One piece of its body: a tapered box from `from` to `from + height`
	// pixels up its length, toppled as it falls. foot in tiles.
	void Part(DrawList& out, float depth, float x, float base, float from, float height, float foot, float taper,
		const Palette::Color& color) const;
	// The colour of its stone, and of one of its cracks.
	Palette::Color Stone(int band) const;
	Alignment CrackColor(int index) const;

	// Its board: the centre square, which squares are pale, and whether a
	// point lies on it.
	void BoardCenter(int& col, int& row) const;
	bool IsPaleSquare(int col, int row) const;
	bool IsOnBoard(float col, float row) const;
	// Keeps a point on its board.
	void ClampToBoard(float& col, float& row) const;

	// Marking the ground it will strike: one square, or a band along a line.
	void MarkSquare(DrawList& out, const ActorContext& ctx, int col, int row, float alpha, Alignment color) const;
	void MarkStrip(DrawList& out, const ActorContext& ctx, float fromCol, float fromRow, float dirCol, float dirRow,
		float length, float width, float progress, Alignment color) const;
	void MarkCircle(DrawList& out, const ActorContext& ctx, float col, float row, float radius, float progress,
		Alignment color) const;

	Origin GetOrigin() const;
	bool HasFallen() const;

private:
	int m_Region;
	Origin m_Origin;
	bool m_Fallen = false;
	float m_FallTime = 0.f;
	float m_FallFacing = 1.f;
};
