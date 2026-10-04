#pragma once

#include "BossActor.h"

// The god's piece (신의 말): not a creature but a piece from its board, a pale
// stone figure set at each gate to guard the way to a king. It moves only as
// a piece may - the light's slides along straight lines like a rook or
// bishop, the dark's leaps two-and-one like a knight - and every third move
// it strikes the board itself.
//
// Its ward turns over every few seconds: burning gold, only dark blows hurt
// it; violet, only light ones. It tests his balance - and whichever gate it
// guards, it leaves light and dark alike, which ought to seem strange.
class GodPieceActor : public BossActor
{
public:
	enum class Kind
	{
		Rook,   // the light's gate
		Knight, // the dark's gate
	};

	GodPieceActor(int region, Kind kind, float maxHealth);

	float BodyRadius() const override;

protected:
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void Reward(const ActorContext& ctx) override;
	float Resist(float damage) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Wait,
		Windup,
		Move,    // sliding or in the air
		Recover,
	};

	enum class Blow
	{
		Slide,
		Leap,
		Board,
	};

	void UpdateWard(float deltaTime);
	void ChooseSlide(float toCol, float toRow);
	void ChooseLeap(float toCol, float toRow);
	void Start(Blow blow, float windup);
	void UpdateMove(const ActorContext& ctx);
	void StrikeBoard(const ActorContext& ctx);
	void Finish();
	bool IsMarkedSquare(int col, int row) const;

	void DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const;

	Kind m_Kind;
	State m_State = State::Wait;
	Blow m_Blow = Blow::Slide;
	float m_StateTime = 0.f;
	float m_Windup = 0.f;
	float m_Cooldown = 1.f;
	int m_MoveCount = 0;
	int m_LeapsLeft = 0;

	// The move under way, fixed when it starts: where from, where to, and
	// whether it has struck him yet.
	float m_FromCol = 0.f;
	float m_FromRow = 0.f;
	float m_ToCol = 0.f;
	float m_ToRow = 0.f;
	bool m_Landed = false;

	// The board strike: its centre square, and which colour of square it hits.
	int m_BoardCol = 0;
	int m_BoardRow = 0;
	int m_BoardParity = 0;

	// Gold wards off the light, violet the dark.
	Alignment m_Ward = Alignment::Light;
	float m_WardTimer = 0.f;

	float m_Lift = 0.f; // pixels off the ground mid-leap
};
