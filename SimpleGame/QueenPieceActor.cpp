#include "stdafx.h"
#include "QueenPieceActor.h"
#include "ThroneRoomActor.h"
#include "EnemyProjectileActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const float kDiagonal = 0.70710678f;

	// The eight lines a queen moves along.
	const float kLineCol[8] = { 1.f, kDiagonal, 0.f, -kDiagonal, -1.f, -kDiagonal, 0.f, kDiagonal };
	const float kLineRow[8] = { 0.f, kDiagonal, 1.f, kDiagonal, 0.f, -kDiagonal, -1.f, -kDiagonal };

	const int kDashSparks = 8;
	const float kDashSparkSpeed = 160.f;
	const float kSparkLift = 30.f;
	const float kStarMark = 1.6f; // tiles round her, marked as the star gathers
}

QueenPieceActor::QueenPieceActor(int region, bool light, bool still)
	: TaintedPieceActor(light ? "흰 퀸 솔레나" : "검은 퀸 녹타", region, light ? Origin::Light : Origin::Dark,
		Tuning::QueenPiece::kMaxHealth, Tuning::QueenPiece::kFuryTime, still ? -1.f : Tuning::RoyalPiece::kWakeRange,
		Tuning::RoyalPiece::kLeashRange, true)
	, m_Light(light)
	, m_Still(still)
{
}

float QueenPieceActor::Scale() const
{
	return Tuning::RoyalPiece::kScale;
}

int QueenPieceActor::BoardHalf() const
{
	return ThroneRoomActor::FloorReach();
}

void QueenPieceActor::Fight(const ActorContext& ctx)
{
	if (m_Still)
	{
		return;
	}
	m_StateTime += ctx.deltaTime;
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}

	float dirCol, dirRow;
	ToPlayer(ctx, dirCol, dirRow);

	switch (m_State)
	{
	case State::Wait:
	{
		FaceAlong(dirCol, dirRow);
		if (m_Cooldown > 0.f)
		{
			break;
		}
		if (m_MoveCount % 3 == 2)
		{
			Start(Blow::Star, Tuning::QueenPiece::kStarWindup);
		}
		else
		{
			m_DashesLeft = Tuning::QueenPiece::kDashes;
			AimDash(ctx);
			Start(Blow::Dash, Tuning::QueenPiece::kDashWindup);
		}
		break;
	}
	case State::Windup:
	{
		if (m_StateTime < m_Windup)
		{
			break;
		}
		if (m_Blow == Blow::Dash)
		{
			// The whole line is struck as she goes; she arrives a moment later.
			Combat::StrikePlayerLine(ctx, WorldX(), WorldY(), m_DirCol, m_DirRow, m_Length,
				Tuning::QueenPiece::kDashWidth, Tuning::QueenPiece::kDashDamage * FuryDamage());
			SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), m_Light ? SparkActor::Style::Light : SparkActor::Style::Dark,
				kDashSparks, kDashSparkSpeed, kSparkLift);
			m_State = State::Dash;
		}
		else
		{
			m_State = State::Star;
			m_RingsLeft = Tuning::QueenPiece::kStarRings;
			m_RingIndex = 0;
			m_NextRing = 0.f;
		}
		m_StateTime = 0.f;
		break;
	}
	case State::Dash:
	{
		float step = m_Length / Tuning::QueenPiece::kDashTime * ctx.deltaTime;
		StepAlong(m_DirCol, m_DirRow, step);
		if (m_StateTime >= Tuning::QueenPiece::kDashTime)
		{
			m_DashesLeft--;
			if (m_DashesLeft > 0)
			{
				// Straight on into the next, aimed afresh.
				AimDash(ctx);
				Start(Blow::Dash, Tuning::QueenPiece::kDashGap + Tuning::QueenPiece::kDashWindup * 0.5f);
			}
			else
			{
				Finish(Tuning::QueenPiece::kCooldown);
			}
		}
		break;
	}
	case State::Star:
	{
		if (m_StateTime >= m_NextRing)
		{
			ThrowRing(ctx);
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::QueenPiece::kCooldown * 0.5f / FurySpeed())
		{
			m_State = State::Wait;
			m_StateTime = 0.f;
		}
		break;
	}
	}
}

void QueenPieceActor::AimDash(const ActorContext& ctx)
{
	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float toCol = playerCol - col;
	float toRow = playerRow - row;
	float distance = sqrtf(toCol * toCol + toRow * toRow);

	// The line closest to the way to him.
	int best = 0;
	float bestDot = -2.f;
	for (int i = 0; i < 8; i++)
	{
		float dot = (distance > 0.f) ? (kLineCol[i] * toCol + kLineRow[i] * toRow) / distance : 0.f;
		if (dot > bestDot)
		{
			bestDot = dot;
			best = i;
		}
	}
	m_DirCol = kLineCol[best];
	m_DirRow = kLineRow[best];

	// Past him a little, but never off the board.
	float length = distance + 2.f;
	if (length > Tuning::QueenPiece::kDashLength)
	{
		length = Tuning::QueenPiece::kDashLength;
	}
	float endCol = col + m_DirCol * length;
	float endRow = row + m_DirRow * length;
	ClampToBoard(endCol, endRow);
	float dc = endCol - col;
	float dr = endRow - row;
	m_Length = sqrtf(dc * dc + dr * dr);
	FaceAlong(m_DirCol, m_DirRow);
}

void QueenPieceActor::Start(Blow blow, float windup)
{
	m_Blow = blow;
	m_Windup = windup / FurySpeed();
	m_State = State::Windup;
	m_StateTime = 0.f;
}

void QueenPieceActor::ThrowRing(const ActorContext& ctx)
{
	// Down all eight lines - each ring turned half a step from the last.
	float turn = (m_RingIndex % 2 == 0) ? 0.f : kPi / 8.f;
	for (int i = 0; i < 8; i++)
	{
		float angle = i * kPi / 4.f + turn;
		std::unique_ptr<EnemyProjectileActor> shot(new EnemyProjectileActor(m_Light ? Alignment::Light : Alignment::Dark,
			cosf(angle), sinf(angle), Tuning::QueenPiece::kStarSpeed * FurySpeed(), 0.f, Tuning::QueenPiece::kStarLifetime,
			Tuning::QueenPiece::kStarDamage * FuryDamage()));
		shot->SetLocalPosition(WorldX(), WorldY());
		ctx.graph->Spawn(std::move(shot));
	}
	m_RingIndex++;
	m_RingsLeft--;
	m_NextRing += Tuning::QueenPiece::kStarGap / FurySpeed();
	if (m_RingsLeft <= 0)
	{
		Finish(Tuning::QueenPiece::kCooldown);
	}
}

void QueenPieceActor::Finish(float cooldown)
{
	m_Cooldown = cooldown / FurySpeed();
	m_MoveCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void QueenPieceActor::OnReset(const ActorContext& ctx)
{
	m_State = State::Wait;
	m_StateTime = 0.f;
	m_Cooldown = 1.2f;
	m_MoveCount = 0;
	m_DashesLeft = 0;
}

void QueenPieceActor::Reward(const ActorContext& ctx)
{
	// The god's piece, whichever side it stood for: light and dark alike.
	ScatterMotes(ctx, Alignment::Light, Tuning::QueenPiece::kRewardMotes);
	ScatterMotes(ctx, Alignment::Dark, Tuning::QueenPiece::kRewardMotes);
}

void QueenPieceActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
	if (m_State != State::Windup)
	{
		return;
	}
	float progress = (m_Windup > 0.f) ? m_StateTime / m_Windup : 1.f;
	if (progress > 1.f)
	{
		progress = 1.f;
	}
	Alignment color = m_Light ? Alignment::Light : Alignment::Dark;
	float col, row;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	if (m_Blow == Blow::Dash)
	{
		MarkStrip(out, ctx, col, row, m_DirCol, m_DirRow, m_Length, Tuning::QueenPiece::kDashWidth, progress, color);
	}
	else
	{
		MarkCircle(out, ctx, col, row, kStarMark, progress, color);
	}
}

void QueenPieceActor::DrawHead(DrawList& out, float depth, float x, float base, float top) const
{
	// A flaring head and a ring of coronet points, a bead at its crown, and
	// her nation's colour round its brow.
	Part(out, depth, x, base, top, 10.f, 0.65f, 1.35f, Stone(0));
	for (int i = -2; i <= 2; i++)
	{
		Part(out, depth + 0.002f * (i + 3), x + i * 3.5f * Tuning::RoyalPiece::kScale, base, top + 10.f, 5.f, 0.08f, 0.3f,
			Stone(0));
	}
	Part(out, depth + 0.02f, x, base, top + 15.f, 4.f, 0.18f, 0.6f, Stone(1));

	float r, g, b;
	AlignmentColor::Core(m_Light ? Alignment::Light : Alignment::Dark, r, g, b);
	Palette::Color band = { r, g, b };
	out.SetGlow(m_Light ? 0.5f : 0.f);
	Part(out, depth + 0.021f, x, base, top + 3.f, 1.5f, 0.7f, 1.f, band);
	out.SetGlow(0.f);
}
