#include "stdafx.h"
#include "Impact.h"
#include "Random.h"
#include <cmath>

namespace
{
	// How fast a jolt dies away, per second. Fast: a blow, not an earthquake.
	const float kShakeDecay = 14.f;
	const float kShakeFloor = 0.2f; // pixels; below this it has stopped
}

void Impact::HitStop(float seconds)
{
	if (m_Enabled && seconds > m_HitStop)
	{
		m_HitStop = seconds;
	}
}

void Impact::Shake(float strength)
{
	if (m_Enabled && strength > m_Shake)
	{
		m_Shake = strength;
	}
}

float Impact::Advance(float deltaTime)
{
	float worldTime = deltaTime;
	if (m_HitStop > 0.f)
	{
		m_HitStop -= deltaTime;
		worldTime = 0.f;
	}

	m_Shake *= expf(-kShakeDecay * deltaTime);
	if (m_Shake < kShakeFloor)
	{
		m_Shake = 0.f;
	}

	m_OffsetX = (Random::Unit() * 2.f - 1.f) * m_Shake;
	m_OffsetY = (Random::Unit() * 2.f - 1.f) * m_Shake;

	return worldTime;
}

float Impact::OffsetX() const
{
	return m_OffsetX;
}

float Impact::OffsetY() const
{
	return m_OffsetY;
}

void Impact::SetEnabled(bool enabled)
{
	m_Enabled = enabled;
	if (!enabled)
	{
		m_HitStop = 0.f;
		m_Shake = 0.f;
		m_OffsetX = 0.f;
		m_OffsetY = 0.f;
	}
}

bool Impact::IsEnabled() const
{
	return m_Enabled;
}
