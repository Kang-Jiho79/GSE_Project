#pragma once

// The weight of a blow, felt through the whole screen: the world stopping for
// an instant as it lands, and the camera jolting. Anything may ask for either;
// the strongest request wins rather than stacking, so a flurry of small hits
// never freezes the game.
class Impact
{
public:
	void HitStop(float seconds);
	// Strength is how far the camera jolts, in pixels; it dies away quickly.
	void Shake(float strength);

	// Real time in, world time out: none while a hit stop holds.
	float Advance(float deltaTime);

	float OffsetX() const;
	float OffsetY() const;

	// Off, requests are ignored and the world runs at its own pace.
	void SetEnabled(bool enabled);
	bool IsEnabled() const;

private:
	bool m_Enabled = true;
	float m_HitStop = 0.f;
	float m_Shake = 0.f;
	float m_OffsetX = 0.f;
	float m_OffsetY = 0.f;
};
