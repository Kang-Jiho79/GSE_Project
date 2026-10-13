#pragma once

#include "BossActor.h"

class WardCoreActor;

// The god (「   」), as he finally meets it: a giant white outline of a man,
// standing beyond the far corner of its own board and leaning over it, the
// pale mark of every altar turning slowly where its face should be. Only its
// two hands come down onto the board, the way a player moves its pieces.
//
// It fights with what it made: first light, then darkness, and last the two
// made white - never the gray, which it did not make and cannot touch. Each
// third of its life is behind a ward of one of them. While a ward stands
// nothing reaches it; the ward's cores stand on the board, and only the
// gray's unravelling (E) undoes one. With all of them gone it bows its face
// to the board for a while, and he can cut it; then the ward rises again,
// until that third of its life is spent and the next ward goes up. A blow can
// never carry it past the next ward.
//
// The hands slam down where he stands and sweep across the board, and its
// face throws its power at him; from the second ward the board's squares
// strike and curses are laid round him; under the last, rings of shot.
//
// Untouchable until it has spoken; beaten, it does not simply vanish but
// slumps over the board, for its last words.
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
		Place,  // a hand comes down where he stands
		Sweep,  // a hand sweeps a line across the board
		Volley, // a fan of its power thrown from its face
		Board,  // every other square strikes
		Mire,   // curses laid round him
		Ring,   // shot from its face, all round
	};

	static const int kHands = 2;
	static const int kMaxCores = 4;

	// 0, 1, 2: which ward is up, by how much of its life is left.
	int PhaseFor(float healthFraction) const;
	// The power of the ward that is up: light, dark, then the god's white.
	Alignment WardAlignment() const;
	bool IsGapOpen() const;

	// The middle of its place, and where its bowed face meets the board.
	void CenterTile(float& col, float& row) const;
	void FacePoint(float& x, float& y) const;

	// The ward's cores set out on the board, or cleared from it.
	void RaiseWard(const ActorContext& ctx);
	void ClearCores();
	int CoresLeft() const;
	void OpenGap(const ActorContext& ctx);

	void Choose(const ActorContext& ctx);
	void Start(Blow blow, float windup);
	void Release(const ActorContext& ctx);
	// Where a hand waits, either side of the board's far corner.
	void RestingPlace(int hand, float& col, float& row) const;
	void UpdateHands(float deltaTime);

	void DrawTelegraph(DrawList& out, const ActorContext& ctx) const;
	void DrawBody(DrawList& out, const ActorContext& ctx) const;
	void DrawHand(DrawList& out, const ActorContext& ctx, int hand) const;
	// The hand's wrist on screen, where its arm meets it.
	void HandScreen(const ActorContext& ctx, int hand, float& x, float& y, float& depth) const;

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

	// The ward: whether one is up, its cores (owned by the graph; only this
	// clears them), and how long the gap stays open once they are gone.
	bool m_WardUp = false;
	WardCoreActor* m_Cores[kMaxCores] = {};
	int m_CoreCount = 0;
	float m_GapTimer = 0.f;
	// 0 standing over the board, 1 bowed to it.
	float m_Bow = 0.f;

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
