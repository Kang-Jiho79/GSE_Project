#pragma once

#include "BossActor.h"

// The god (「   」), as he finally meets it: an empty place torn in the sky -
// neither light nor dark, only nothing - and two hands circling its rim,
// coming down onto the board the way a player moves its pieces.
//
// Three wards in turn, one for each power he holds, in the order he came by
// them: first only light reaches it, then only darkness, and last only the
// gray. Each third of its life is behind one ward; a blow can never carry it
// past the next one. The hands slam down where he stands and sweep across the
// board; from the second ward on, the board's squares themselves strike; under
// the last, the empty place spits rings of shot.
//
// Untouchable until it has spoken; beaten, it does not simply vanish but
// breaks, for its last words.
class GodActor : public BossActor
{
public:
	using BossActor::TakeHit;

	explicit GodActor(int region);

	float BodyRadius() const override;
	float TakeHit(float damage) override;
	const char* Hint() const override;

	void Provoke();
	bool HasFallen() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void Fight(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void Reward(const ActorContext& ctx) override;
	void Defeated(const ActorContext& ctx) override;
	float Resist(float damage) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Hover,
		Windup,
		Recover,
		Shift, // turning its ward over to the next power
	};

	enum class Blow
	{
		Place, // a hand comes down where he stands
		Sweep, // a hand sweeps a line across the board
		Board, // every other square strikes
		Ring,  // shot from the empty place, all round
	};

	static const int kHands = 2;

	// 0, 1, 2: which ward is up, by how much of its life is left.
	int PhaseFor(float healthFraction) const;
	Form WardForm() const;
	Alignment WardAlignment() const;

	void Choose(const ActorContext& ctx);
	void Start(Blow blow, float windup);
	void Release(const ActorContext& ctx);
	// Where a hand floats while it waits, circling the empty place.
	void RestingPlace(int hand, float& col, float& row) const;
	void UpdateHands(float deltaTime);

	void DrawTelegraph(DrawList& out, const ActorContext& ctx) const;
	void DrawVoid(DrawList& out, float depth, float x, float groundY) const;
	void DrawHand(DrawList& out, const ActorContext& ctx, int hand) const;

	bool m_Provoked = false;
	bool m_Fallen = false;
	float m_FallenTime = 0.f;
	float m_Clock = 0.f; // its own, running even before the fight

	int m_Phase = 0;
	State m_State = State::Hover;
	Blow m_Blow = Blow::Place;
	float m_StateTime = 0.f;
	float m_Windup = 0.f;
	float m_Cooldown = 1.5f;
	int m_BlowCount = 0;
	int m_ActiveHand = 0;
	float m_StrikeFlash = 0.f;

	// The blow under way, fixed when it starts: where, and which way a sweep
	// runs; the board's centre square and which colour of square it strikes.
	float m_TargetCol = 0.f;
	float m_TargetRow = 0.f;
	float m_SweepCol = 1.f;
	float m_SweepRow = 0.f;
	int m_BoardCol = 0;
	int m_BoardRow = 0;
	int m_BoardParity = 0;

	// Each hand's place on the ground beneath it, and how high it floats.
	float m_HandCol[kHands] = {};
	float m_HandRow[kHands] = {};
	float m_HandLift[kHands] = {};
	bool m_HandsPlaced = false;
};
