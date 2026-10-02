#pragma once

#include "Actor.h"

// The light form's called lightning. Marks the ground first, so anything that
// sees it coming can step out, then strikes everything still inside.
class LightningActor : public Actor
{
public:
	LightningActor();

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	void DrawWarning(DrawList& out, float x, float groundY) const;
	void DrawStrike(DrawList& out, float x, float groundY) const;

	float m_Age = 0.f;
	bool m_Struck = false;

	// Sideways kinks of the bolt, rolled once so it does not flicker.
	static const int kKinks = 5;
	float m_Kinks[kKinks];
};
