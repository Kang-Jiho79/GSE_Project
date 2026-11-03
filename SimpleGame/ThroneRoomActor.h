#pragma once

#include "Actor.h"
#include "Regions.h"
#include "Settlement.h"

// The throne room inside a capital's castle, a map of its own: a hall walled
// round, its floor a board of pale and dark squares - even here - two rows of
// pillars, a carpet from the door up to the dais and the throne on it, and
// the land's banners and lights - gold windows in the light's castle, violet
// braziers in the dark's. The king stands at the carpet's end, the queen
// beside the throne.
class ThroneRoomActor : public Actor
{
public:
	// Lays out its walls and pillars - and the blockers that go with them.
	explicit ThroneRoomActor(Regions::Style style);

	// Where the king stands and where the carpet runs, as a settlement's.
	static Settlement::Layout Layout();
	// The floor, in squares from the room's centre on both axes, and that
	// centre.
	static int FloorReach();
	static void Center(float& col, float& row);
	// Where he comes in, and the door he goes out by.
	static void Arrival(float& col, float& row);
	static void ExitDoor(float& col, float& row);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	bool Place(const ActorContext& ctx, float col, float row, float& x, float& y, float& depth) const;

	Regions::Style m_Style;
	float m_Time = 0.f;
};
