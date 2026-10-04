#pragma once

#include "Settlement.h"
#include "NpcActor.h"

class SceneGraph;
class KinisActor;
class GodActor;
struct Progress;
struct PlayerStatus;

// The story's events out in the world, between the prologue and the end:
//
// - On the old battlefield, the first time: Kinis, waiting, who tells him
//   the two kings plotted together, and then is gone.
// - In two villages near the gray's heart - Fides on the light's side,
//   Requiem on the dark's - someone of that side surrounded by the gray.
//   Saved, each kneels at the broken altar and prays, and he overhears it.
//   Once he has heard both, he finds the two prayers are one; whether that
//   sets him free of the god depends on whether he has seen both kings fail
//   to hear him, and whether he is strong enough.
// - If he was not, then later, once he is, in any village near the gray's
//   heart, it comes back to him.
// - Free of the god, at the front: the two he saved, at each other's throats
//   again for no reason they can give.
// - At the gray's heart, Kinis. Short of the true ending's conditions - both
//   kings' recognition and the god's hold broken - they talk first, and he
//   offers to take the gray together; accepted, that is the end. Otherwise
//   they fight, and his last words come broken or whole, and the ending
//   follows from them - and, for one who heard them whole, from whether he
//   takes up the gray Kinis leaves.
//
// Owned by the scene, which sets it up for each region and asks it each
// frame whether a conversation should start.
class StoryEvents
{
public:
	void Enter(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph,
		float centerCol, float centerRow);

	// Moves it on. Returns a conversation to start now, or -1.
	int Update(SceneGraph& graph, Progress& progress, const PlayerStatus& status, float playerX, float playerY);

	// While someone is being saved, how many of the gray are left round them;
	// otherwise -1.
	int RescueLeft() const;

	// An ending reached, as a Dialogue::EndingId, or -1. Taken once.
	int TakeEnding();
	// From the moment Kinis is spoken to, nothing is saved: whatever happens,
	// continuing goes back to arriving at the gray's heart.
	bool HoldsSave() const;
	// A region the story sends him to now, or -1. Taken once.
	int TakeTravel();

private:
	enum class Stage
	{
		None,
		Kinis,        // waiting for him to come near
		KinisLeaving, // has spoken; goes as soon as the conversation ends
		Rescue,       // the gray still round the one to be saved
		Rescued,      // has spoken; becomes an ordinary survivor
		Reunion,      // the two he saved, waiting at the front
		ReunionOver,  // they go as soon as the conversation ends
		KinisWaiting, // at the gray's heart, until he comes near
		KinisTalked,  // the conversation before the fight is over
		KinisFight,
		KinisLastWords,
		GodWaiting,   // in the god's place, as he arrives
		GodTalked,
		GodFight,
		GodFalling,   // has spoken its last; gone as soon as that ends
		Done,
	};

	bool TrueEndReached(const Progress& progress) const;
	NpcActor* SpawnPerson(SceneGraph& graph, NpcActor::Kind kind, Regions::Style style, int conversation, float col, float row);
	void Dissolve(SceneGraph& graph, Actor& actor) const;

	bool IsNearGray() const;
	bool IsLightSide() const;
	float DistanceTo(const Actor& actor, float playerX, float playerY) const;
	void SpawnRaiders(SceneGraph& graph, float col, float row) const;

	int m_Region = 0;
	Stage m_Stage = Stage::None;
	bool m_Remembered = false;
	int m_Left = 0;

	// Whoever the event is about: Kinis on the battlefield, the one being
	// saved, or the two at the front. Owned by the graph; only this ever
	// destroys them, and everything goes with the region.
	NpcActor* m_Figure = nullptr;
	NpcActor* m_Second = nullptr; // the other of the two at the front
	KinisActor* m_Kinis = nullptr;
	GodActor* m_God = nullptr;
	int m_Ending = -1;
	int m_Travel = -1;
};
