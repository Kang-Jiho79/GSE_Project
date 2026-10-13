#include "stdafx.h"
#include "PawnSoldierActor.h"
#include "ParticleActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "Palette.h"
#include "Random.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kHitFlashTime = 0.12f;
	const int kMotes = 3;
	const float kMoteSpeed = 120.f;
	const int kDeathDust = 10;
	const float kStrikeShake = 2.f;

	// A small black piece gone gray.
	const Palette::Color kStone = { 0.22f, 0.21f, 0.24f };
}

PawnSoldierActor::PawnSoldierActor(int col, int row, int endRow)
	: m_Col(col)
	, m_Row(row)
	, m_EndRow(endRow)
	, m_Health(Tuning::TaintedPawn::kSoldierHealth)
	, m_Cooldown(Tuning::TaintedPawn::kSoldierTurn * (0.5f + Random::Unit() * 0.5f))
{
	PlaceOn((float)col, (float)row);
}

void PawnSoldierActor::PlaceOn(float col, float row)
{
	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);
}

float PawnSoldierActor::TakeHit(float damage)
{
	if (m_Dead)
	{
		return 0.f;
	}
	float landed = (damage < m_Health) ? damage : m_Health;
	m_Health -= damage;
	m_HitFlash = 1.f;
	if (m_Health <= 0.f)
	{
		m_Dead = true;
		m_MotesPending = true;
	}
	return landed;
}

float PawnSoldierActor::BodyRadius() const
{
	return Tuning::TaintedPawn::kSoldierRadius;
}

bool PawnSoldierActor::IsDead() const
{
	return m_Dead;
}

bool PawnSoldierActor::ReachedEnd() const
{
	return !m_Dead && m_Row >= m_EndRow && m_State == State::Wait;
}

void PawnSoldierActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;
	if (m_HitFlash > 0.f)
	{
		m_HitFlash -= ctx.deltaTime / kHitFlashTime;
	}

	// Fallen: what it was scattered for him, once.
	if (m_MotesPending)
	{
		m_MotesPending = false;
		for (int i = 0; i < kMotes; i++)
		{
			float angle = Random::Angle();
			std::unique_ptr<ParticleActor> mote(new ParticleActor(Alignment::Dark, cosf(angle) * kMoteSpeed,
				sinf(angle) * kMoteSpeed));
			mote->SetLocalPosition(WorldX(), WorldY());
			ctx.graph->Spawn(std::move(mote));
		}
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dust, kDeathDust, 120.f, 16.f);
	}
	if (m_Dead || m_Row >= m_EndRow)
	{
		return;
	}

	m_StateTime += ctx.deltaTime;
	float playerCol, playerRow;
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);

	switch (m_State)
	{
	case State::Wait:
	{
		m_Cooldown -= ctx.deltaTime;
		if (m_Cooldown > 0.f)
		{
			break;
		}
		float dc = playerCol - m_Col;
		float dr = playerRow - m_Row;
		if (dc * dc + dr * dr <= Tuning::TaintedPawn::kSoldierStrikeReach * Tuning::TaintedPawn::kSoldierStrikeReach)
		{
			m_State = State::Windup;
			m_StateTime = 0.f;
			break;
		}
		// He stands on the square ahead: it cannot step.
		float ac = playerCol - m_Col;
		float ar = playerRow - (m_Row + 1);
		if (ac * ac + ar * ar <= Tuning::TaintedPawn::kBlockReach * Tuning::TaintedPawn::kBlockReach)
		{
			m_Cooldown = Tuning::TaintedPawn::kSoldierTurn * 0.5f;
			break;
		}
		m_State = State::Step;
		m_StateTime = 0.f;
		break;
	}
	case State::Step:
	{
		float t = m_StateTime / Tuning::TaintedPawn::kStepTime;
		if (t >= 1.f)
		{
			m_Row++;
			PlaceOn((float)m_Col, (float)m_Row);
			m_State = State::Wait;
			m_Cooldown = Tuning::TaintedPawn::kSoldierTurn;
			break;
		}
		PlaceOn((float)m_Col, m_Row + t);
		break;
	}
	case State::Windup:
	{
		if (m_StateTime < Tuning::TaintedPawn::kSoldierWindup)
		{
			break;
		}
		// Both forward diagonals at once.
		for (int side = -1; side <= 1; side += 2)
		{
			float x, y;
			WorldGrid::IsoPosition((float)(m_Col + side), (float)(m_Row + 1), x, y);
			Combat::StrikePlayerRadius(ctx, x, y, Tuning::TaintedPawn::kStrikeRadius, Tuning::TaintedPawn::kSoldierDamage);
		}
		ctx.impact->Shake(kStrikeShake);
		m_State = State::Wait;
		m_Cooldown = Tuning::TaintedPawn::kSoldierTurn;
		break;
	}
	}
}

void PawnSoldierActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	if (m_Dead)
	{
		return;
	}
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 60.f, 80.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	// The two squares it will take, marked while it gathers.
	if (m_State == State::Windup)
	{
		float progress = m_StateTime / Tuning::TaintedPawn::kSoldierWindup;
		float r, g, b;
		AlignmentColor::Warning(Alignment::Dark, Palette::kWarningWash, r, g, b);
		for (int side = -1; side <= 1; side += 2)
		{
			int col = m_Col + side;
			int row = m_Row + 1;
			float worldX, worldY, sx, sy;
			WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);
			ctx.ToScreen(worldX, worldY, sx, sy);
			out.AddDiamond((float)(col + row) + 0.03f, sx, sy + WorldGrid::GroundHeightAt(worldX, worldY),
				WorldGrid::kTileWidth - 6.f, WorldGrid::kTileHeight - 3.f, r, g, b, 0.15f + 0.3f * progress);
		}
	}

	out.AddShadow(depth - 0.01f, x, groundY, 34.f, 17.f);
	out.SetFigure(true);
	if (m_HitFlash > 0.f)
	{
		out.SetFlash(m_HitFlash * 0.75f);
	}
	// A small pawn: a foot, a short column, and a round head.
	float bob = (m_State == State::Step) ? sinf(m_StateTime / Tuning::TaintedPawn::kStepTime * 3.14159f) * 4.f : 0.f;
	float base = groundY + bob;
	out.AddTaperedBox(depth, x, base, 0.8f, 0.8f, 4.f, 0.85f, 0.f, 0.f, kStone.r * 0.9f, kStone.g * 0.9f, kStone.b * 0.9f);
	out.AddTaperedBox(depth + 0.005f, x, base + 4.f, 0.45f, 0.45f, 14.f, 0.75f, 0.f, 0.f, kStone.r, kStone.g, kStone.b);
	out.AddTaperedBox(depth + 0.01f, x, base + 18.f, 0.4f, 0.4f, 8.f, 1.2f, 0.f, 0.f, kStone.r, kStone.g, kStone.b);
	out.AddTaperedBox(depth + 0.015f, x, base + 26.f, 0.48f, 0.48f, 5.f, 0.4f, 0.f, 0.f,
		kStone.r * 1.1f, kStone.g * 1.1f, kStone.b * 1.1f);
	out.SetFlash(0.f);
	out.SetFigure(false);

	// A violet crack across it.
	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);
	out.AddRect(depth + 0.02f, x + 2.f, base + 12.f, 1.4f, 7.f, r, g, b, 1.f);
}
