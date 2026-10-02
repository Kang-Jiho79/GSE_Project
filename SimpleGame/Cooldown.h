#pragma once

// A countdown that remembers how long it was, so a gauge can show how far
// through it is rather than just whether it has finished.
struct Cooldown
{
	float remaining = 0.f;
	float duration = 0.f;

	void Start(float seconds)
	{
		remaining = seconds;
		duration = seconds;
	}

	void Tick(float deltaTime)
	{
		if (remaining > 0.f)
		{
			remaining -= deltaTime;
			if (remaining < 0.f)
			{
				remaining = 0.f;
			}
		}
	}

	bool Ready() const
	{
		return remaining <= 0.f;
	}

	// 0 the moment it starts, 1 once it is ready again.
	float Progress() const
	{
		if (duration <= 0.f)
		{
			return 1.f;
		}
		return 1.f - remaining / duration;
	}
};
