#include "stdafx.h"
#include "MonsterModels.h"
#include "DrawList.h"
#include "Alignment.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;

	// Gray armour, the light side's a shade paler than the dark's.
	const float kLightArmor = 0.31f;
	const float kDarkArmor = 0.23f;
	const float kSteel = 0.52f;

	// What is left of a power in them: faded and dimmed, but still telling.
	void Remnant(Alignment alignment, float& r, float& g, float& b)
	{
		AlignmentColor::Core(alignment, r, g, b);
		float gray = (r + g + b) / 3.f;
		r = (r + (gray - r) * 0.45f) * 0.75f;
		g = (g + (gray - g) * 0.45f) * 0.75f;
		b = (b + (gray - b) * 0.45f) * 0.75f;
	}

	float Lerp(float from, float to, float t)
	{
		return from + (to - from) * t;
	}

	float Smooth(float t)
	{
		return t * t * (3.f - 2.f * t);
	}

	// A straight blade from the hand, narrowing to the tip. The angle is on
	// screen, measured from the way the body faces: 0 straight ahead, 90 up,
	// -90 down, 180 behind.
	void Blade(DrawList& out, float depth, float handX, float handY, float facing,
		float angleDegrees, float length, float width, float shade, float& tipX, float& tipY)
	{
		float angle = angleDegrees * kDegToRad;
		float dirX = cosf(angle) * facing;
		float dirY = sinf(angle);

		tipX = handX + dirX * length;
		tipY = handY + dirY * length;

		float sideX = -dirY * width * 0.5f;
		float sideY = dirX * width * 0.5f;

		float corners[8] =
		{
			handX - sideX, handY - sideY,
			tipX - sideX * 0.4f, tipY - sideY * 0.4f,
			tipX + sideX * 0.4f, tipY + sideY * 0.4f,
			handX + sideX, handY + sideY,
		};
		out.AddCorners(depth, corners, shade, shade, shade * 1.05f, 1.f);
	}

	// Two straight pieces with a bend between them, read as a curved edge.
	void CurvedBlade(DrawList& out, float depth, float handX, float handY, float facing,
		float angleDegrees, float length, float width, float bendDegrees, float shade)
	{
		float midX, midY, tipX, tipY;
		Blade(out, depth, handX, handY, facing, angleDegrees, length * 0.55f, width, shade, midX, midY);
		Blade(out, depth, midX, midY, facing, angleDegrees + bendDegrees, length * 0.5f, width * 0.7f, shade, tipX, tipY);
	}
}

void MonsterModels::DrawLightKnight(DrawList& out, const Pose& pose)
{
	float f = pose.facing;
	float d = pose.depth;
	float x = pose.x;

	float stride = pose.moving ? sinf(pose.gait) * 3.f : 0.f;
	float bob = pose.moving ? fabsf(sinf(pose.gait)) * 1.2f : sinf(pose.time * 2.f) * 0.4f;

	// Rocks back while gathering the blow, then throws its weight into it.
	float lean = (-pose.windup * 2.f + pose.strike * 4.f) * f;

	float rr, rg, rb;
	Remnant(Alignment::Light, rr, rg, rb);

	out.AddShadow(d, x, pose.groundY, 34.f, 17.f);

	// Straight legs, planted square.
	const float kLegHeight = 14.f;
	out.AddTaperedBox(d, x - 5.f + stride, pose.groundY, 0.15f, 0.15f, kLegHeight,
		0.92f, 0.f, 0.f, kLightArmor * 0.8f, kLightArmor * 0.8f, kLightArmor * 0.8f);
	out.AddTaperedBox(d, x + 5.f - stride, pose.groundY, 0.15f, 0.15f, kLegHeight,
		0.92f, 0.f, 0.f, kLightArmor * 0.8f, kLightArmor * 0.8f, kLightArmor * 0.8f);

	// An upright breastplate, only a little wider at the shoulder.
	float torsoBase = pose.groundY + kLegHeight + bob;
	const float kTorsoHeight = 20.f;
	out.AddTaperedBox(d + 0.01f, x, torsoBase, 0.30f, 0.26f, kTorsoHeight,
		1.12f, lean, 0.f, kLightArmor, kLightArmor, kLightArmor);

	// The sun-sign it once wore, still faintly gold.
	out.AddDiamond(d + 0.015f, x + f * 2.f + lean * 0.5f, torsoBase + kTorsoHeight * 0.55f,
		6.f, 8.f, rr, rg, rb, 1.f);

	// Square pauldrons.
	float shoulderY = torsoBase + kTorsoHeight - 3.f;
	out.AddBox(d + 0.02f, x - 12.f + lean, shoulderY, 0.15f, 0.15f, 5.f,
		kLightArmor * 1.1f, kLightArmor * 1.1f, kLightArmor * 1.1f);
	out.AddBox(d + 0.02f, x + 12.f + lean, shoulderY, 0.15f, 0.15f, 5.f,
		kLightArmor * 1.1f, kLightArmor * 1.1f, kLightArmor * 1.1f);

	// A squared helm with a slit of old gold for eyes.
	float headBase = torsoBase + kTorsoHeight;
	float headX = x + f + lean;
	out.AddTaperedBox(d + 0.03f, headX, headBase, 0.2f, 0.2f, 11.f,
		0.94f, 0.f, 0.f, kLightArmor * 1.1f, kLightArmor * 1.1f, kLightArmor * 1.1f);
	out.AddRect(d + 0.04f, headX + f * 3.5f, headBase + 6.f, 5.f, 1.4f, rr, rg, rb, 1.f);

	// The halo it still wears: a flat ring floating over the helm.
	out.AddGroundArc(pose.worldX, pose.worldY, headX, headBase + 16.f, 0.30f, 0.38f,
		0.f, 6.2831853f, 16, rr, rg, rb, 0.55f);

	// The sword, in the far hand: hanging at rest, raised high and back while
	// it gathers, brought down forward as the blow lands.
	float swordAngle = -70.f;
	if (pose.windup > 0.f)
	{
		swordAngle = Lerp(-70.f, 115.f, Smooth(pose.windup));
	}
	else if (pose.strike > 0.f)
	{
		swordAngle = Lerp(-70.f, -15.f, pose.strike);
	}

	float handX = x - f * 11.f + lean;
	float handY = torsoBase + 8.f;
	float tipX, tipY;
	Blade(out, d + 0.045f, handX, handY, f, swordAngle, 22.f, 3.5f, kSteel, tipX, tipY);
	out.AddRect(d + 0.046f, handX, handY, 6.f, 2.f,
		kLightArmor * 0.8f, kLightArmor * 0.8f, kLightArmor * 0.8f, 1.f);

	// The shield, a tall slab held on the near side toward what it fights.
	float shieldX = x + f * 12.f + lean;
	float shieldY = torsoBase + 7.f;
	out.AddRect(d + 0.05f, shieldX, shieldY, 11.f, 24.f,
		kLightArmor * 1.15f, kLightArmor * 1.15f, kLightArmor * 1.15f, 1.f);
	out.AddRect(d + 0.051f, shieldX, shieldY, 8.f, 21.f,
		kLightArmor * 0.85f, kLightArmor * 0.85f, kLightArmor * 0.85f, 1.f);
	out.AddDiamond(d + 0.052f, shieldX, shieldY + 2.f, 5.f, 7.f, rr, rg, rb, 1.f);
}

void MonsterModels::DrawDarkKnight(DrawList& out, const Pose& pose)
{
	float f = pose.facing;
	float d = pose.depth;
	float x = pose.x;

	float stride = pose.moving ? sinf(pose.gait * 1.2f) * 3.5f : 0.f;
	float bob = pose.moving ? fabsf(sinf(pose.gait * 1.2f)) * 1.5f : sinf(pose.time * 3.f) * 0.5f;

	// Always hunched forward; more while gathering, flat out in a charge.
	float lean = f * (3.f + pose.windup * 2.f + pose.strike * 4.f + (pose.charging ? 7.f : 0.f));

	float rr, rg, rb;
	Remnant(Alignment::Dark, rr, rg, rb);

	out.AddShadow(d, x, pose.groundY, 36.f, 18.f);

	// Short legs set wide, knees pushed forward into a crouch.
	const float kLegHeight = 10.f;
	out.AddTaperedBox(d, x - 7.f + stride, pose.groundY, 0.14f, 0.14f, kLegHeight,
		0.8f, f * 2.f, 0.f, kDarkArmor * 0.8f, kDarkArmor * 0.8f, kDarkArmor * 0.8f);
	out.AddTaperedBox(d, x + 7.f - stride, pose.groundY, 0.14f, 0.14f, kLegHeight,
		0.8f, f * 2.f, 0.f, kDarkArmor * 0.8f, kDarkArmor * 0.8f, kDarkArmor * 0.8f);

	// A narrow waist flaring hard to broad, barbed shoulders.
	float torsoBase = pose.groundY + kLegHeight + bob;
	const float kTorsoHeight = 15.f;
	out.AddTaperedBox(d + 0.01f, x + lean * 0.3f, torsoBase, 0.24f, 0.22f, kTorsoHeight,
		1.4f, lean, 0.f, kDarkArmor, kDarkArmor, kDarkArmor);

	// A slash of old violet across the chest.
	out.AddRect(d + 0.015f, x + f * 2.f + lean * 0.6f, torsoBase + kTorsoHeight * 0.5f,
		1.6f, 7.f, rr, rg, rb, 1.f);

	// Spikes raking back and out off each shoulder.
	float shoulderY = torsoBase + kTorsoHeight - 2.f;
	out.AddTaperedBox(d + 0.02f, x - 13.f + lean, shoulderY, 0.08f, 0.08f, 8.f,
		0.1f, -3.f - f * 2.f, 0.f, kDarkArmor * 1.15f, kDarkArmor * 1.15f, kDarkArmor * 1.15f);
	out.AddTaperedBox(d + 0.02f, x + 13.f + lean, shoulderY, 0.08f, 0.08f, 8.f,
		0.1f, 3.f - f * 2.f, 0.f, kDarkArmor * 1.15f, kDarkArmor * 1.15f, kDarkArmor * 1.15f);

	// The head carried low and ahead of the shoulders, horns sweeping back.
	float headX = x + f * 4.f + lean * 1.2f;
	float headBase = torsoBase + kTorsoHeight - 3.f;
	out.AddTaperedBox(d + 0.03f, headX, headBase, 0.17f, 0.17f, 8.f,
		0.85f, f, 0.f, kDarkArmor * 1.1f, kDarkArmor * 1.1f, kDarkArmor * 1.1f);
	out.AddTaperedBox(d + 0.035f, headX - 4.f, headBase + 6.f, 0.05f, 0.05f, 9.f,
		0.12f, -4.f - f * 3.f, 0.f, kDarkArmor * 1.3f, kDarkArmor * 1.3f, kDarkArmor * 1.3f);
	out.AddTaperedBox(d + 0.035f, headX + 4.f, headBase + 6.f, 0.05f, 0.05f, 9.f,
		0.12f, 4.f - f * 3.f, 0.f, kDarkArmor * 1.3f, kDarkArmor * 1.3f, kDarkArmor * 1.3f);

	float eyeX = headX + f * 3.f;
	out.AddRect(d + 0.04f, eyeX - 1.8f, headBase + 4.f, 1.6f, 1.4f, rr, rg, rb, 1.f);
	out.AddRect(d + 0.04f, eyeX + 1.8f, headBase + 4.f, 1.6f, 1.4f, rr, rg, rb, 1.f);

	// A curved blade in each hand: trailing low behind at rest, swept up and
	// back while it gathers, carved forward as the blow lands, dragged behind
	// in a charge.
	float bladeAngle = -110.f;
	if (pose.charging)
	{
		bladeAngle = -160.f;
	}
	else if (pose.windup > 0.f)
	{
		bladeAngle = Lerp(-110.f, 150.f, Smooth(pose.windup));
	}
	else if (pose.strike > 0.f)
	{
		bladeAngle = Lerp(-110.f, -15.f, pose.strike);
	}

	float handY = torsoBase + 5.f;
	CurvedBlade(out, d - 0.005f, x - f * 10.f + lean, handY, f, bladeAngle + 20.f, 20.f, 3.f, 35.f, kSteel * 0.85f);
	CurvedBlade(out, d + 0.045f, x + f * 10.f + lean, handY, f, bladeAngle, 20.f, 3.f, 35.f, kSteel);
}

void MonsterModels::DrawLightMage(DrawList& out, const Pose& pose)
{
	float f = pose.facing;
	float d = pose.depth;
	float x = pose.x;

	// It glides rather than walks; the robe hides any stride.
	float bob = pose.moving ? fabsf(sinf(pose.gait)) * 1.f : sinf(pose.time * 1.6f) * 0.5f;
	float lean = (pose.windup * 1.5f + pose.strike * 2.f) * f;

	const float kRobe = 0.34f;
	float rr, rg, rb;
	Remnant(Alignment::Light, rr, rg, rb);

	out.AddShadow(d, x, pose.groundY, 26.f, 13.f);

	// A long robe, wide at the hem, rising narrow to the chest.
	const float kRobeHeight = 24.f;
	out.AddTaperedBox(d, x, pose.groundY, 0.32f, 0.30f, kRobeHeight,
		0.55f, lean, 0.f, kRobe, kRobe, kRobe);
	out.AddDiamond(d + 0.005f, x + f * 2.f + lean * 0.7f, pose.groundY + kRobeHeight * 0.7f,
		5.f, 7.f, rr, rg, rb, 1.f);

	// Upright shoulders, and pauldrons swept up and out like folded wings.
	float shoulderBase = pose.groundY + kRobeHeight - 2.f + bob;
	out.AddTaperedBox(d + 0.01f, x + lean, shoulderBase, 0.18f, 0.16f, 5.f,
		1.3f, 0.f, 0.f, kRobe * 1.05f, kRobe * 1.05f, kRobe * 1.05f);
	out.AddTaperedBox(d + 0.02f, x - 9.f + lean, shoulderBase + 3.f, 0.07f, 0.12f, 10.f,
		0.2f, -6.f, 0.f, kRobe * 1.15f, kRobe * 1.15f, kRobe * 1.15f);
	out.AddTaperedBox(d + 0.02f, x + 9.f + lean, shoulderBase + 3.f, 0.07f, 0.12f, 10.f,
		0.2f, 6.f, 0.f, kRobe * 1.15f, kRobe * 1.15f, kRobe * 1.15f);

	// A tall pointed hood, the face a shadow with two points of old gold.
	float headBase = shoulderBase + 5.f;
	float headX = x + f + lean;
	out.AddTaperedBox(d + 0.03f, headX, headBase, 0.17f, 0.17f, 13.f,
		0.25f, -f, 0.f, kRobe * 1.05f, kRobe * 1.05f, kRobe * 1.05f);
	out.AddRect(d + 0.035f, headX + f * 3.f, headBase + 4.5f, 6.f, 5.f, 0.08f, 0.08f, 0.09f, 1.f);
	out.AddRect(d + 0.04f, headX + f * 3.f - 1.5f, headBase + 4.5f, 1.2f, 1.2f, rr, rg, rb, 1.f);
	out.AddRect(d + 0.04f, headX + f * 3.f + 1.5f, headBase + 4.5f, 1.2f, 1.2f, rr, rg, rb, 1.f);

	// A staff in the far hand. While it chants the staff tips forward and
	// the orb at its head swells with light.
	float staffAngle = 90.f - 25.f * Smooth(pose.windup);
	float handX = x - f * 11.f + lean;
	float handY = pose.groundY + 3.f + pose.windup * 4.f;
	float tipX, tipY;
	Blade(out, d + 0.04f, handX, handY, f, staffAngle, 34.f, 2.5f, 0.38f, tipX, tipY);

	float charge = pose.windup + pose.strike;
	float glowR, glowG, glowB;
	AlignmentColor::Glow(Alignment::Light, glowR, glowG, glowB);
	out.AddGlow(d + 0.045f, tipX, tipY + 3.f, 10.f + 14.f * charge, glowR, glowG, glowB,
		0.3f + 0.6f * (charge > 1.f ? 1.f : charge));
	out.AddDiamond(d + 0.05f, tipX, tipY + 3.f, 7.f, 9.f, rr, rg, rb, 1.f);
}

void MonsterModels::DrawDarkMage(DrawList& out, const Pose& pose)
{
	float f = pose.facing;
	float d = pose.depth;
	float x = pose.x;

	// It hangs a little off the ground, drifting, its hem trailing.
	float hover = 7.f + sinf(pose.time * 2.2f) * 2.f;
	float sway = sinf(pose.time * 3.1f) * 2.f;
	float lean = f * (2.f + pose.windup * 2.f + pose.strike * 2.f);
	float baseY = pose.groundY + hover;

	float rr, rg, rb;
	Remnant(Alignment::Dark, rr, rg, rb);

	// A smaller, fainter shadow, since it is not standing on it.
	out.AddShadow(d, x, pose.groundY, 22.f, 11.f);

	// A ragged hem: strips hanging below the cloak, narrowing to points.
	for (int strip = -1; strip <= 1; strip++)
	{
		out.AddTaperedBox(d, x + strip * 6.f, baseY - 7.f, 0.06f, 0.06f, 8.f,
			2.2f, -sway * (0.6f + 0.3f * strip), 0.f, kDarkArmor * 0.8f, kDarkArmor * 0.8f, kDarkArmor * 0.8f);
	}

	// The cloak: narrow at the shoulders, flaring as it falls.
	const float kCloakHeight = 20.f;
	out.AddTaperedBox(d + 0.01f, x, baseY, 0.30f, 0.28f, kCloakHeight,
		0.5f, lean, 0.f, kDarkArmor, kDarkArmor, kDarkArmor);

	// A low hood pushed forward, swallowing the face but for the eyes.
	float headBase = baseY + kCloakHeight - 2.f;
	float headX = x + f * 3.f + lean;
	out.AddTaperedBox(d + 0.03f, headX, headBase, 0.19f, 0.19f, 9.f,
		0.45f, f * 3.f, 0.f, kDarkArmor * 1.1f, kDarkArmor * 1.1f, kDarkArmor * 1.1f);
	out.AddRect(d + 0.035f, headX + f * 3.f, headBase + 3.5f, 6.f, 4.f, 0.05f, 0.05f, 0.06f, 1.f);
	out.AddRect(d + 0.04f, headX + f * 3.f - 1.5f, headBase + 3.5f, 1.3f, 1.3f, rr, rg, rb, 1.f);
	out.AddRect(d + 0.04f, headX + f * 3.f + 1.5f, headBase + 3.5f, 1.3f, 1.3f, rr, rg, rb, 1.f);

	// Hands held out in front, an orb of dark gathering between them as it
	// casts - always a faint ember, swelling with the spell.
	float handX = x + f * 10.f + lean;
	float handY = baseY + 10.f;
	out.AddBox(d + 0.04f, handX - 3.f, handY, 0.05f, 0.05f, 3.f,
		kDarkArmor * 1.2f, kDarkArmor * 1.2f, kDarkArmor * 1.2f);
	out.AddBox(d + 0.04f, handX + 3.f, handY, 0.05f, 0.05f, 3.f,
		kDarkArmor * 1.2f, kDarkArmor * 1.2f, kDarkArmor * 1.2f);

	float charge = pose.windup + pose.strike;
	if (charge > 1.f)
	{
		charge = 1.f;
	}
	float orbY = handY + 5.f;
	float glowR, glowG, glowB;
	AlignmentColor::Glow(Alignment::Dark, glowR, glowG, glowB);
	out.AddGlow(d + 0.045f, handX, orbY, 10.f + 16.f * charge, glowR, glowG, glowB, 0.4f + 0.6f * charge);
	float size = 4.f + 8.f * charge;
	out.AddDiamond(d + 0.05f, handX, orbY, size, size * 0.75f, rr, rg, rb, 1.f);
}
