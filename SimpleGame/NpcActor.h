#pragma once

#include "Actor.h"
#include "Regions.h"

// Someone who lives in a settlement and will talk to him: a villager, an
// elder, a child, or a temple's priest - or Kinis, met once on the road.
// Stands where it was put, breathing, and turns to face him as he comes
// near. Dressed in its people's way.
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
	};

	// conversation: the Dialogue::Id it starts when spoken to, or -1 for
	// someone who is not spoken to with F.
	NpcActor(Kind kind, Regions::Style style, int conversation);

	int Conversation() const;
	void SetConversation(int conversation);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void DrawKinis(DrawList& out, float depth, float x, float groundY) const;

	Kind m_Kind;
	Regions::Style m_Style;
	int m_Conversation;
	float m_Facing = 1.f;
	float m_Time = 0.f;
};
