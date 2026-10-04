#pragma once

#include "Actor.h"

class DrawList;

// Owns the actor tree and drives it: one traversal updates everything, another
// collects everything to draw. Actors spawned or destroyed mid-traversal are
// only linked in or unlinked once that traversal is finished, so the tree is
// never edited while it is being walked.
class SceneGraph
{
public:
	SceneGraph();

	Actor& Root();

	// Where Spawn() hangs new actors. Defaults to the root.
	void SetSpawnParent(Actor* parent);
	void Spawn(std::unique_ptr<Actor> actor);

	// Drops everything - the whole tree and anything waiting to join it - for
	// a new level to be built in its place. Not to be called mid-traversal.
	void Clear();

	void Update(const ActorContext& ctx);
	void Draw(DrawList& out, const ActorContext& ctx);

	// Finds every actor of a given type, wherever it sits in the tree.
	template <typename T, typename F>
	void ForEach(F fn)
	{
		m_Root.VisitAll<T>(fn);
	}

private:
	void IntegratePending();

	Actor m_Root;
	Actor* m_SpawnParent;
	std::vector<std::unique_ptr<Actor>> m_Pending;
};
