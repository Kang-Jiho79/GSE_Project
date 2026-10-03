#pragma once

class DrawList;

// The eroded soldiers' bodies, built from tapered boxes like everything else.
// All of them gray - the gray took them - with only a trace of their old power
// left in the eyes and an emblem, and a silhouette that still says which side
// they fought for: light upright and squared off, dark low and barbed.
namespace MonsterModels
{
	// Everything a body needs to be posed this frame.
	struct Pose
	{
		float x = 0.f;        // feet on screen
		float groundY = 0.f;
		float depth = 0.f;
		float worldX = 0.f;   // feet in the world, for pieces laid flat like a halo
		float worldY = 0.f;
		float facing = 1.f;   // +1 toward the right of the screen, -1 the left
		float gait = 0.f;     // walk phase, radians
		bool moving = false;
		float time = 0.f;
		float windup = 0.f;   // 0..1 while gathering for a blow
		float strike = 0.f;   // 1 the instant a blow lands, easing back to 0
		bool charging = false;
	};

	void DrawLightKnight(DrawList& out, const Pose& pose);
	void DrawDarkKnight(DrawList& out, const Pose& pose);
	void DrawLightMage(DrawList& out, const Pose& pose);
	void DrawDarkMage(DrawList& out, const Pose& pose);
}
