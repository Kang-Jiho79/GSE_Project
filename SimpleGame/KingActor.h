#pragma once

#include "BossActor.h"

struct PlayerStatus;

// What the two kings share. A king does not fight until he asks for a duel at
// the end of the carpet; then he is made whole, held to the king's own power,
// and the king fights him with nothing else.
//
// Beaten, he is not killed: the king lowers its weapon, leaves him a little
// health, and he may ask again. Walk far enough away and the duel is simply
// over. Win, and the king yields - kneels, acknowledges him, blesses him -
// and a moment later he is sent on to Bellum. From then on the king only
// stands at its palace.
class KingActor : public BossActor
{
public:
	using BossActor::TakeHit;

	// What it says: when he asks, when it spares him, when he walks away, when
	// it yields, and the blessing it gives, written under the yielding - and
	// the conversation it has with him once it has acknowledged him.
	struct Lines
	{
		const char* challenge;
		const char* spare;
		const char* flee;
		const char* yield;
		const char* blessing;
		int talk;
	};

	// recognized: it has already yielded to him, and will not fight again.
	KingActor(const char* name, int region, float maxHealth, float furyTime, Form power, const Lines& lines,
		bool recognized);

	// Nothing he does touches a king outside a duel.
	float TakeHit(float damage) override;

	// The form the duel holds him to.
	Form Power() const;
	bool CanChallenge() const;
	// Having acknowledged him on an earlier visit, it will talk instead.
	bool CanTalk() const;
	int Conversation() const;
	bool IsDueling() const;
	bool HasYielded() const;
	// He has asked: he is made whole and held to its power, and it wakes.
	// His form must already be its power.
	void BeginDuel(PlayerStatus& status);

	// True once, a while after it yields: time to send him on.
	bool TakeSendOff();

	// Its last words while they are still shown - nullptr once they fade -
	// and what is written under them.
	const char* Remark() const;
	const char* Aside() const;
	float RemarkAlpha() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnReset(const ActorContext& ctx) override;
	void Defeated(const ActorContext& ctx) override;

	// Each king's own: forget whatever move was under way.
	virtual void ResetMoves() = 0;

	// 0 the moment it yields, rising to 1 as it goes down on one knee.
	float Kneel() const;

private:
	void EndDuel(PlayerStatus& status);
	void Say(const char* line, const char* aside);

	Form m_Power;
	Lines m_Lines;
	bool m_Recognized;
	bool m_Dueling = false;

	bool m_Yielded = false;
	float m_YieldTime = 0.f;
	bool m_SendOffPending = false;
	bool m_SentOff = false;

	const char* m_Remark = nullptr;
	const char* m_Aside = nullptr;
	float m_RemarkTimer = 0.f;
};
