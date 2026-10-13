#pragma once

#include <vector>
#include "Cutscenes.h"

class Renderer;
class TextRenderer;

// Plays one of the Cutscenes shot by shot. While it plays it owns the
// camera - where it looks and how close - and says how fast the world under
// it runs; the scene asks it each frame and does the rest: keeping his hands
// off the controls, hiding the HUD, and acting on the cues it hands over.
//
// Holding F, Space or Enter skips ahead - to the next shot that opens a
// conversation, or to the end - and every cue passed on the way is still
// handed over, so the story never misses a step.
class Cutscene
{
public:
	// anchorX/Y: the world point shot offsets are measured from. startX/Y:
	// where the camera is now, so the first shot eases from there.
	void Play(int id, float anchorX, float anchorY, float startX, float startY);
	bool IsPlaying() const;

	// Real seconds, not world time: a stopped world does not stop the camera.
	// playerX/Y: where he stands now, for shots that look at him.
	void Update(float deltaTime, float playerX, float playerY, bool skipHeld);

	float CameraX() const;
	float CameraY() const;
	float Zoom() const;
	// What the world's time is multiplied by under the current shot.
	float TimeScale() const;

	// Cues and a conversation the shots have begun, each taken once; -1 when
	// there are none left.
	int TakeCue();
	int TakeConversation();

	// The bars and the wash, over the world. Drawn at no zoom.
	void Render(Renderer* renderer, int windowSizeX, int windowSizeY) const;
	// The caption in the lower bar, and how far a held skip has got.
	void RenderText(TextRenderer* text, int windowSizeX, int windowSizeY) const;

private:
	void BeginShot(int index);
	void SkipAhead();
	void Target(const Cutscenes::Shot& shot, float& x, float& y) const;
	const Cutscenes::Shot& Current() const;

	const Cutscenes::Script* m_Script = nullptr;
	bool m_Playing = false;
	int m_Index = 0;
	float m_ShotTime = 0.f;

	float m_AnchorX = 0.f;
	float m_AnchorY = 0.f;
	float m_PlayerX = 0.f;
	float m_PlayerY = 0.f;

	// The camera now, and where the current shot set out from.
	float m_X = 0.f;
	float m_Y = 0.f;
	float m_Zoom = 1.f;
	float m_FromX = 0.f;
	float m_FromY = 0.f;
	float m_FromZoom = 1.f;

	float m_Letterbox = 0.f; // 0..1, easing in and out
	float m_SkipHeld = 0.f;  // seconds the skip has been held

	std::vector<int> m_Cues;
	int m_Conversation = -1;
};
