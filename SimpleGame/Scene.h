#pragma once

#include "SceneGraph.h"
#include "DrawList.h"

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

private:
	void BuildLevel();
	ActorContext MakeContext(float deltaTime, PlayerStatus& status);

	SceneGraph m_Graph;
	DrawList m_DrawList;

	// Owned by the graph; kept only because the camera and the monsters both
	// need to know where he is.
	PlayerActor* m_Player = nullptr;

	int m_WindowSizeX = 0;
	int m_WindowSizeY = 0;
};
