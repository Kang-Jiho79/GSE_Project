#pragma once

#include "KingActor.h"

// Noctis, the dark's king (어둠의 왕 녹티스). Fought in dark form alone, so
// the fight is one of reach - and lifesteal. He is a schemer and knows it:
// he does not close in but makes him come, and punishes him for it.
//
// The longer he stays close, feeding, the readier the counter - a stance
// that turns every blow aside and answers it far harder - and the ground
// round the king is sown with snares. Keep away instead, and shades rise
// beside him and dark shot bends after him. Worn down, the king thinks
// faster: shorter windups, snares more often - and he makes his reckoning:
// three likenesses of him circling the hall with him, trading places. Only
// he casts a shadow. Strike him and the reckoning is broken and he reels;
// strike a likeness and it bursts on him, holding him fast, and the king
// drinks from it. Too long, and all four come at him at once.
class NoctisActor : public KingActor
{
public:
	NoctisActor(int region, bool recognized);

	float BodyRadius() const override;
	const char* Hint() const override;

	// A likeness of his burst on him: the king drinks from it.
	void Drink(float amount);
	// Drawn as he stands now, for a likeness of him: no shadow under it.
	void DrawLikeness(DrawList& out, float depth, float x, float groundY) const;

protected:
	void Fight(const ActorContext& ctx) override;
	void ResetMoves() override;
	void Reward(const ActorContext& ctx) override;
	float Resist(float damage) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Stalk,
		Windup,
		Lunge,   // across the ground
		Counter, // the stance, waiting to be struck
		Riposte, // struck in it: the answer coming
		Recover,
		Reckon,  // circling with his likenesses
		Stagger, // the reckoning broken under him
	};

	enum class Blow
	{
		Cut,
		Lunge,
		Counter,
		Snares,
		Shades,
		Orbs,
	};

	bool Cunning() const;
	void Stalk(const ActorContext& ctx, float distance, float dirCol, float dirRow);
	void Choose(const ActorContext& ctx, float distance, float dirCol, float dirRow);
	void Start(Blow blow, float windup, float dirCol, float dirRow);
	void Release(const ActorContext& ctx);
	void LaySnares(const ActorContext& ctx);
	void RaiseShades(const ActorContext& ctx);
	void ThrowOrbs(const ActorContext& ctx);
	void StartReckoning(const ActorContext& ctx);
	void UpdateReckoning(const ActorContext& ctx, float dirCol, float dirRow);
	int SnaresLaid(const ActorContext& ctx) const;
	void Finish();

	void DrawTelegraph(DrawList& out, float x, float groundY) const;
	void DrawBody(DrawList& out, float depth, float x, float groundY, bool shadow) const;

	State m_State = State::Stalk;
	Blow m_Blow = Blow::Cut;
	Blow m_LastBlow = Blow::Cut;
	float m_StateTime = 0.f;
	float m_Windup = 0.f;
	float m_Cooldown = 1.f;
	float m_ShadeCooldown = 0.f;
	int m_BlowCount = 0;

	// The reckoning: when he may make it again, whether he has been found
	// out, how far round the hall it has turned, which of the four places
	// is his, and until they trade places.
	float m_ReckonCooldown = 0.f;
	bool m_ReckonBroken = false;
	float m_ReckonTurn = 0.f;
	int m_RealSlot = 0;
	float m_SwapTimer = 0.f;
	float m_StaggerTime = 0.f;

	// Seconds he has been kept close, slowly forgotten while he keeps away.
	float m_Pressure = 0.f;
	bool m_Countered = false;

	// The blow under way, aimed when it starts.
	float m_DirCol = 1.f;
	float m_DirRow = 0.f;

	float m_StrafeSign = 1.f;
	float m_StrafeTimer = 0.f;

	float m_StrikeFlash = 0.f;
	bool m_Moving = false;
	float m_Gait = 0.f;
};
