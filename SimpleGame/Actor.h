#pragma once

#include <memory>
#include <vector>

class DrawList;
class SceneGraph;
class Impact;
struct PlayerStatus;

// What the player is asking for this frame, in screen terms. The camera only
// translates, so a screen direction is already a world direction.
struct InputState
{
	float moveRight = 0.f; // -1..1
	float moveUp = 0.f;    // -1..1

	// Held: true every frame the button is down.
	bool attack = false;
	bool run = false;

	// Pressed: true only on the frame the key went down, so holding it does
	// not repeat the action.
	bool switchForm = false;
	bool dodge = false;
	bool skill = false;   // E
	bool special = false; // Q

	float aimX = 0.f; // mouse relative to the window center
	float aimY = 0.f;
};

// Everything an actor is allowed to know about the frame it is being updated
// or drawn in. Passing this instead of handing actors a pointer to the whole
// scene is what keeps them independent of each other.
struct ActorContext
{
	float deltaTime = 0.f;

	float camX = 0.f;
	float camY = 0.f;
	float playerX = 0.f;
	float playerY = 0.f;

	int windowSizeX = 0;
	int windowSizeY = 0;

	InputState input;
	PlayerStatus* status = nullptr;
	SceneGraph* graph = nullptr;
	Impact* impact = nullptr;

	void ToScreen(float worldX, float worldY, float& outX, float& outY) const;
	bool IsOnScreen(float screenX, float screenY, float padX, float padY) const;
};

// A node in the scene graph, and the only thing the world is built out of.
// Position is local to the parent; the world position is resolved during
// traversal, so moving a parent carries its children with it.
//
// A plain Actor with no overrides is a useful thing on its own: a group node.
class Actor
{
public:
	Actor();
	virtual ~Actor();

	Actor* AddChild(std::unique_ptr<Actor> child);

	void SetLocalPosition(float x, float y);
	float LocalX() const;
	float LocalY() const;
	float WorldX() const;
	float WorldY() const;

	// Removal is deferred: an actor may be destroyed in the middle of a
	// traversal, so it is only unlinked once the traversal is over.
	void Destroy();
	bool IsDestroyed() const;

	void UpdateTree(const ActorContext& ctx, float parentWorldX, float parentWorldY);
	void DrawTree(DrawList& out, const ActorContext& ctx, float parentWorldX, float parentWorldY) const;
	void PruneDestroyed();

	// Visits this actor and every descendant that is a T. This is how one actor
	// finds another without either of them knowing where it lives in the graph.
	template <typename T, typename F>
	void VisitAll(F& fn)
	{
		T* typed = dynamic_cast<T*>(this);
		if (typed != nullptr)
		{
			fn(*typed);
		}

		for (size_t i = 0; i < m_Children.size(); i++)
		{
			m_Children[i]->VisitAll<T>(fn);
		}
	}

protected:
	virtual void OnUpdate(const ActorContext& ctx);
	virtual void OnDraw(DrawList& out, const ActorContext& ctx) const;

private:
	float m_LocalX = 0.f;
	float m_LocalY = 0.f;

	// A cache of local resolved against every parent above. Mutable because the
	// draw traversal refreshes it too - an actor spawned after the last update
	// has never had it resolved, and must still draw in the right place.
	mutable float m_WorldX = 0.f;
	mutable float m_WorldY = 0.f;

	bool m_Destroyed = false;

	std::vector<std::unique_ptr<Actor>> m_Children;
};
