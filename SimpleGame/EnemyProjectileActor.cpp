#include "stdafx.h"
#include "EnemyProjectileActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kFlightHeight = 14.f; // pixels; the height bolts fly at
	const float kFadeTime = 0.2f;     // fades out over the end of its life
	const float kPi = 3.14159265f;
}

EnemyProjectileActor::EnemyProjectileActor(Alignment alignment, float dirCol, float dirRow,
	float speed, float turnRate, float lifetime, float damage)
	: m_Alignment(alignment)
	, m_DirCol(dirCol)
	, m_DirRow(dirRow)
	, m_Speed(speed)
	, m_TurnRate(turnRate)
	, m_Lifetime(lifetime)
	, m_Damage(damage)
{
}

void EnemyProjectileActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_Age >= m_Lifetime)
	{
		Destroy();
		return;
	}

	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);

	if (m_TurnRate > 0.f)
	{
		float playerCol, playerRow;
		WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
		BendToward(playerCol - col, playerRow - row, ctx.deltaTime);
	}

	col += m_DirCol * m_Speed * ctx.deltaTime;
	row += m_DirRow * m_Speed * ctx.deltaTime;
	WorldGrid::ClampToMap(col, row);

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);

	if (Combat::StrikePlayerRadius(ctx, x, y, Tuning::Monster::kProjectileHitRadius, m_Damage))
	{
		Destroy();
	}
}

void EnemyProjectileActor::BendToward(float toCol, float toRow, float deltaTime)
{
	if (toCol * toCol + toRow * toRow < 0.0001f)
	{
		return;
	}

	float current = atan2f(m_DirRow, m_DirCol);
	float wanted = atan2f(toRow, toCol);

	float turn = wanted - current;
	while (turn > kPi)
	{
		turn -= 2.f * kPi;
	}
	while (turn < -kPi)
	{
		turn += 2.f * kPi;
	}

	float limit = m_TurnRate * deltaTime;
	if (turn > limit)
	{
		turn = limit;
	}
	else if (turn < -limit)
	{
		turn = -limit;
	}

	current += turn;
	m_DirCol = cosf(current);
	m_DirRow = sinf(current);
}

void EnemyProjectileActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 40.f, 40.f))
	{
		return;
	}

	float baseY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + kFlightHeight;
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.3f;

	float fade = 1.f;
	if (m_Lifetime - m_Age < kFadeTime)
	{
		fade = (m_Lifetime - m_Age) / kFadeTime;
	}

	float glowR, glowG, glowB;
	AlignmentColor::Glow(m_Alignment, glowR, glowG, glowB);
	float r, g, b;
	AlignmentColor::Core(m_Alignment, r, g, b);

	if (m_Alignment == Alignment::Light)
	{
		// The player's own light, gone ashen: paler, with a dark rim, so it is
		// never mistaken for one of his bolts.
		AlignmentColor::Warning(m_Alignment, Palette::kHostileSpellWash, r, g, b);

		out.AddGlow(depth - 0.01f, x, baseY, 22.f, glowR, glowG, glowB, 0.8f * fade);
		out.AddDiamond(depth, x, baseY, 16.f, 11.f, Palette::kHostileRim.r, Palette::kHostileRim.g, Palette::kHostileRim.b, fade);
		out.SetGlow(0.7f);
		out.AddDiamond(depth + 0.001f, x, baseY, 12.f, 8.f, r, g, b, fade);
		return;
	}

	// A swelling orb of dark, its halo sinking the ground beneath it.
	float pulse = 1.f + 0.15f * sinf(m_Age * 9.f);
	out.AddGlow(depth - 0.01f, x, baseY, 30.f * pulse, glowR, glowG, glowB, fade);
	out.AddDiamond(depth, x, baseY, 15.f * pulse, 11.f * pulse, r * 0.7f, g * 0.7f, b * 0.7f, fade);
	out.AddDiamond(depth + 0.001f, x, baseY, 6.f, 4.f, 0.78f, 0.70f, 1.f, fade);
}
