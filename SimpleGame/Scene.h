#pragma once

#include "SceneGraph.h"
#include "DrawList.h"
#include "Impact.h"

class Renderer;
class PlayerActor;
struct PlayerStatus;

// Level 1. Builds the actor tree, hands it the frame, and turns what it
// collected into draw calls. All the world's behaviour lives in the actors;
// this owns only the level layout, the camera and the viewport.
class Scene
{
public:
	Scene();

	void SetViewport(int windowSizeX, int windowSizeY);

	void Update(const InputState& input, float deltaTime, PlayerStatus& status);
	void Render(Renderer* renderer, PlayerStatus& status);
	void RenderMinimap(Renderer* renderer);

	// Sprays, hit stops and screen shake, together.
	void SetEffectsEnabled(bool enabled);
	bool AreEffectsEnabled() const;

private:
	void BuildLevel();
	ActorContext MakeContext(float deltaTime, PlayerStatus& status);
	void RecordPopulation();

	// Death: the world fades out, he is moved back to the start while it is
	// fully dark, and it fades back in.
	void UpdateDeath(float deltaTime, PlayerStatus& status);
	void DrawDeathFade(Renderer* renderer) const;
	// The world washing toward gray as his balance goes.
	void DrawErosionTint(Renderer* renderer, const PlayerStatus& status) const;

	SceneGraph m_Graph;
	DrawList m_DrawList;
	Impact m_Impact;

	// Owned by the graph; kept only because the camera and the monsters both
	// need to know where he is.
	PlayerActor* m_Player = nullptr;

	int m_WindowSizeX = 0;
	int m_WindowSizeY = 0;

	// Seconds since he fell; negative while he is alive.
	float m_DeathElapsed = -1.f;
};
