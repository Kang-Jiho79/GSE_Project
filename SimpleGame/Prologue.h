#pragma once

class SceneGraph;
class Impact;
struct Progress;

// The hidden village's last day, played out the first time he is there: the
// story so far, a little while at peace among the people who raised him, then
// the gray comes out of the woods. The villagers are lost to it whatever he
// does; when the last of the gray falls he is left alone, and the waystone
// wakes so he can go after it.
//
// Owned by the scene, which asks it each frame whether a conversation should
// start.
class Prologue
{
public:
	// The village as it was, from the top.
	void Begin();
	// Not here, or already over.
	void Stop();
	bool IsActive() const;

	// The waystone stays dark until it is over.
	bool HoldsWaystone() const;
	// While the gray is in the village, how many are left; otherwise -1.
	int RaidersLeft() const;

	// Moves it on. Returns a conversation to start now, or -1.
	int Update(float deltaTime, SceneGraph& graph, Progress& progress, Impact& impact,
		float centerCol, float centerRow);

private:
	enum class Stage
	{
		Idle,
		Opening,
		Calm,
		Raid,
		Aftermath,
	};

	void SpawnRaiders(SceneGraph& graph, float centerCol, float centerRow);
	void TakeVillagers(SceneGraph& graph);

	Stage m_Stage = Stage::Idle;
	float m_Time = 0.f;
	bool m_Taken = false;
	int m_Left = 0;
};
