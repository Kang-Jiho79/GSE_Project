#include "stdafx.h"
#include "TaintedBishopActor.h"
#include "EnemyProjectileActor.h"
#include "BeamActor.h"
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
#include <cstdlib>

namespace
{
	const float kInvRoot2 = 0.70710678f;
	// The four diagonals.
	const int kDiagCol[4] = { 1, 1, -1, -1 };
	const int kDiagRow[4] = { 1, -1, 1, -1 };

	const int kBlinkSparks = 14;
	const float kStrikeShake = 4.f;
	const float kLatticeShake = 7.f;

	int Mod(int value, int by)
	{
		return ((value % by) + by) % by;
	}
}

TaintedBishopActor::TaintedBishopActor(int region)
	: TaintedPieceActor("잠식된 비숍", region, Origin::Light, Tuning::TaintedBishop::kMaxHealth,
		Tuning::TaintedBishop::kFuryTime)
{
}

const char* TaintedBishopActor::Hint() const
{
	return OnPale() ? "흰 칸 · 대각선의 빛" : "검은 칸 · 대각선의 어둠";
}

void TaintedBishopActor::Square(int& col, int& row) const
{
	float c, r;
	WorldGrid::InverseIso(WorldX(), WorldY(), c, r);
	col = (int)floorf(c + 0.5f);
	row = (int)floorf(r + 0.5f);
}

bool TaintedBishopActor::OnPale() const
{
	int col, row;
	Square(col, row);
	return IsPaleSquare(col, row);
}

bool TaintedBishopActor::IsLatticeSquare(int col, int row) const
{
	int line = m_LatticeRising ? col + row : col - row;
	return Mod(line, Tuning::TaintedBishop::kLatticeSpacing) != m_LatticeSafe;
}

float TaintedBishopActor::WindupTime() const
{
	float windup = Tuning::TaintedBishop::kBlinkWindup;
	switch (m_Blow)
	{
	case Blow::Beam:
	{
		windup = Tuning::TaintedBishop::kBeamWindup;
		break;
	}
	case Blow::Orb:
	{
		windup = Tuning::TaintedBishop::kOrbWindup;
		break;
	}
	case Blow::Shift:
	{
		windup = Tuning::TaintedBishop::kShiftWindup;
		break;
	}
	case Blow::Lattice:
	{
		windup = Tuning::TaintedBishop::kLatticeWindup;
		break;
	}
	default:
	{
		break;
	}
	}
	return windup / FurySpeed();
}

void TaintedBishopActor::Fight(const ActorContext& ctx)
{
	m_StateTime += ctx.deltaTime;

	switch (m_State)
	{
	case State::Wait:
	{
		float dirCol, dirRow;
		ToPlayer(ctx, dirCol, dirRow);
		FaceAlong(dirCol, dirRow);
		m_Cooldown -= ctx.deltaTime;
		if (m_Cooldown <= 0.f)
		{
			Choose(ctx);
		}
		break;
	}
	case State::Windup:
	{
		if (m_StateTime >= WindupTime())
		{
			Release(ctx);
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::TaintedBishop::kRecover / FurySpeed())
		{
			m_State = State::Wait;
			m_StateTime = 0.f;
			m_Cooldown = Tuning::TaintedBishop::kCooldown / FurySpeed();
		}
		break;
	}
	}
}

void TaintedBishopActor::Choose(const ActorContext& ctx)
{
	int col, row;
	Square(col, row);
	float playerCol, playerRow;
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);

	bool lateHalf = HealthFraction() < Tuning::TaintedBishop::kLatticeShare;
	int every = Tuning::TaintedBishop::kLatticeEvery;
	if (lateHalf && m_MoveCount % every == every - 1)
	{
		m_Blow = Blow::Lattice;
		m_LatticeRising = (m_LatticeCount % 2 == 0);
		m_LatticeSafe = Random::Range(0, Tuning::TaintedBishop::kLatticeSpacing);
		m_LatticeCount++;
	}
	else if (m_MoveCount % Tuning::TaintedBishop::kShiftEvery == Tuning::TaintedBishop::kShiftEvery - 1)
	{
		// The step a bishop may never take: straight, toward him.
		m_Blow = Blow::Shift;
		float dc = playerCol - col;
		float dr = playerRow - row;
		m_ToCol = col;
		m_ToRow = row;
		if (fabsf(dc) >= fabsf(dr))
		{
			m_ToCol += (dc >= 0.f) ? 1 : -1;
		}
		else
		{
			m_ToRow += (dr >= 0.f) ? 1 : -1;
		}
		if (!IsOnBoard((float)m_ToCol, (float)m_ToRow))
		{
			m_ToCol = col - (m_ToCol - col);
			m_ToRow = row - (m_ToRow - row);
		}
	}
	else if (m_MoveCount % 2 == 0)
	{
		// Along a diagonal to the square that puts him most nearly on one
		// of its own diagonals, not right on top of him.
		m_Blow = Blow::Blink;
		float best = 1e9f;
		m_ToCol = col;
		m_ToRow = row;
		for (int d = 0; d < 4; d++)
		{
			for (int step = 1; step <= Tuning::TaintedBishop::kBlinkReach; step++)
			{
				int c = col + kDiagCol[d] * step;
				int r = row + kDiagRow[d] * step;
				if (!IsOnBoard((float)c, (float)r) || WorldGrid::IsBlocked((float)c, (float)r))
				{
					break;
				}
				float dc = playerCol - c;
				float dr = playerRow - r;
				float span = sqrtf(dc * dc + dr * dr);
				if (span < 2.f)
				{
					continue;
				}
				float offLine = fabsf(fabsf(dc) - fabsf(dr));
				float score = offLine + span * 0.1f;
				if (score < best)
				{
					best = score;
					m_ToCol = c;
					m_ToRow = r;
				}
			}
		}
	}
	else
	{
		m_Blow = OnPale() ? Blow::Beam : Blow::Orb;
	}

	m_State = State::Windup;
	m_StateTime = 0.f;
}

void TaintedBishopActor::Release(const ActorContext& ctx)
{
	switch (m_Blow)
	{
	case Blow::Blink:
	case Blow::Shift:
	{
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Light, kBlinkSparks, 180.f, 30.f);
		float x, y;
		WorldGrid::IsoPosition((float)m_ToCol, (float)m_ToRow, x, y);
		SetLocalPosition(x, y);
		SparkActor::Burst(*ctx.graph, x, y, (m_Blow == Blow::Shift) ? SparkActor::Style::Dust : SparkActor::Style::Light,
			kBlinkSparks, 180.f, 30.f);
		break;
	}
	case Blow::Beam:
	{
		for (int d = 0; d < 4; d++)
		{
			float dirCol = kDiagCol[d] * kInvRoot2;
			float dirRow = kDiagRow[d] * kInvRoot2;
			Combat::StrikePlayerLine(ctx, WorldX(), WorldY(), dirCol, dirRow, Tuning::TaintedBishop::kBeamLength,
				Tuning::TaintedBishop::kBeamWidth, Tuning::TaintedBishop::kBeamDamage * FuryDamage());
			std::unique_ptr<BeamActor> beam(new BeamActor(Alignment::Light, dirCol, dirRow, Tuning::TaintedBishop::kBeamLength));
			beam->SetLocalPosition(WorldX(), WorldY());
			ctx.graph->Spawn(std::move(beam));
		}
		ctx.impact->Shake(kStrikeShake);
		break;
	}
	case Blow::Orb:
	{
		for (int d = 0; d < 4; d++)
		{
			std::unique_ptr<EnemyProjectileActor> orb(new EnemyProjectileActor(Alignment::Dark,
				kDiagCol[d] * kInvRoot2, kDiagRow[d] * kInvRoot2, Tuning::TaintedBishop::kOrbSpeed * FurySpeed(), 0.f,
				Tuning::TaintedBishop::kOrbLifetime, Tuning::TaintedBishop::kOrbDamage * FuryDamage()));
			orb->SetLocalPosition(WorldX(), WorldY());
			ctx.graph->Spawn(std::move(orb));
		}
		break;
	}
	case Blow::Lattice:
	{
		int playerCol, playerRow;
		WorldGrid::WorldToTile(ctx.playerX, ctx.playerY, playerCol, playerRow);
		if (IsOnBoard((float)playerCol, (float)playerRow) && IsLatticeSquare(playerCol, playerRow))
		{
			Combat::StrikePlayerRadius(ctx, ctx.playerX, ctx.playerY, 1.f, Tuning::TaintedBishop::kLatticeDamage * FuryDamage());
		}
		ctx.impact->Shake(kLatticeShake);
		break;
	}
	}

	m_MoveCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void TaintedBishopActor::OnReset(const ActorContext& ctx)
{
	m_State = State::Wait;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
	m_MoveCount = 0;
}

void TaintedBishopActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
	if (m_State != State::Windup)
	{
		return;
	}
	float progress = m_StateTime / WindupTime();
	if (progress > 1.f)
	{
		progress = 1.f;
	}
	float col, row;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);

	switch (m_Blow)
	{
	case Blow::Blink:
	case Blow::Shift:
	{
		MarkCircle(out, ctx, (float)m_ToCol, (float)m_ToRow, 0.6f, progress, Alignment::Light);
		break;
	}
	case Blow::Beam:
	case Blow::Orb:
	{
		Alignment color = (m_Blow == Blow::Beam) ? Alignment::Light : Alignment::Dark;
		float width = (m_Blow == Blow::Beam) ? Tuning::TaintedBishop::kBeamWidth : Tuning::TaintedBishop::kBeamWidth * 0.6f;
		for (int d = 0; d < 4; d++)
		{
			MarkStrip(out, ctx, col, row, kDiagCol[d] * kInvRoot2, kDiagRow[d] * kInvRoot2,
				Tuning::TaintedBishop::kBeamLength, width, progress, color);
		}
		break;
	}
	case Blow::Lattice:
	{
		int centerCol, centerRow;
		BoardCenter(centerCol, centerRow);
		int half = Tuning::TaintedPiece::kBoardHalf;
		float alpha = 0.12f + 0.3f * progress;
		for (int r = centerRow - half; r <= centerRow + half; r++)
		{
			for (int c = centerCol - half; c <= centerCol + half; c++)
			{
				if (IsLatticeSquare(c, r))
				{
					MarkSquare(out, ctx, c, r, alpha, Alignment::Light);
				}
			}
		}
		break;
	}
	}
}

void TaintedBishopActor::DrawHead(DrawList& out, float depth, float x, float base, float top) const
{
	// A collar, a tall mitre narrowing to a point, split down its front, and
	// a small knob on top.
	Part(out, depth, x, base, top, 4.f, 0.5f, 1.3f, Stone(1));
	Part(out, depth + 0.005f, x, base, top + 4.f, 20.f, 0.58f, 0.4f, Stone(0));
	Part(out, depth + 0.01f, x, base, top + 24.f, 4.f, 0.16f, 0.6f, Stone(1));
	if (HasFallen())
	{
		return;
	}
	float r, g, b;
	AlignmentColor::Core(Alignment::Light, r, g, b);
	out.SetGlow(0.6f);
	out.AddRect(depth + 0.02f, x + Facing() * 3.f, base + top + 13.f, 1.6f, 11.f, r, g, b, 1.f);
	out.SetGlow(0.f);
}
