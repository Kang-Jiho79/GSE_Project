#pragma once

#include "Actor.h"
#include "Regions.h"

// Someone who lives in a settlement and will talk to him: a villager, an
// elder, a child, or a temple's priest - or Kinis, met once on the road.
// Stands where it was put, breathing, and turns to face him as he comes
// near. Dressed in its people's way.
//
// Or something left lying about to be read the same way, with F: a
// signpost, a note or an inscription, a lost lantern.
class NpcActor : public Actor
{
public:
	enum class Kind
	{
		Adult,
		Elder,
		Child,
		Priest,
		Kinis,
		Queen,  // a king's queen, at his throne's side
		// A peddler with a pack, who begs for help while the gray is round
		// him and thanks him once it is gone.
		Merchant,

		// Things, not people.
		Sign,    // a leaning signpost
		Note,    // a scrap of writing, or words cut in stone
		Lantern, // a lost lantern, gone once found
	};

	// conversation: the Dialogue::Id it starts when spoken to, or -1 for
	// someone who is not spoken to with F.
	NpcActor(Kind kind, Regions::Style style, int conversation);

	int Conversation() const;
	void SetConversation(int conversation);
	// Read rather than spoken to.
	bool IsReadable() const;
	Kind Type() const;
	// Whose people it is dressed as.
	Regions::Style Dress() const;

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void DrawKinis(DrawList& out, float depth, float x, float groundY) const;
	void DrawThing(DrawList& out, float depth, float x, float groundY) const;

	Kind m_Kind;
	Regions::Style m_Style;
	int m_Conversation;
	float m_Facing = 1.f;
	float m_Time = 0.f;
};
