#pragma once

#include "Actor.h"

class SceneGraph;

// A small spray thrown off by something happening - sparks where a blow
// lands, a shower as the form turns over, a puff of dust under a roll. One
// actor carries the whole spray, so a busy fight does not mean hundreds of
// actors.
class SparkActor : public Actor
{
public:
	enum class Style
	{
		Light, // gold streaks that shine
		Dark,  // violet streaks that do not
		Dust,  // gray puffs low on the ground
	};

	// Throws a spray at a world point. Does nothing while effects are off.
	// speed is in pixels per second; lift is how high off the ground it starts.
	static void Burst(SceneGraph& graph, float worldX, float worldY, Style style,
		int count, float speed, float lift);

	static void SetEnabled(bool enabled);
	static bool IsEnabled();

	SparkActor(Style style, int count, float speed, float lift);

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	float Lifetime() const;

	static const int kMaxSparks = 16;

	Style m_Style;
	int m_Count;
	float m_Lift;
	float m_Age = 0.f;

	// Each spark's direction on screen and its own speed, rolled once.
	float m_DirX[kMaxSparks];
	float m_DirY[kMaxSparks];
	float m_Speed[kMaxSparks];
};
