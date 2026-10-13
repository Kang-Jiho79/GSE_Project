#include "stdafx.h"
#include "TaintedPawnActor.h"
#include "PawnSoldierActor.h"
#include "BurstActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kStrikeShake = 4.f;
	const float kPromoteShake = 8.f;
	const int kPromoteSparks = 24;

	// The eight lines a queen may slide along.
	const float kInvRoot2 = 0.70710678f;
	const float kLineCol[8] = { 1.f, -1.f, 0.f, 0.f, kInvRoot2, kInvRoot2, -kInvRoot2, -kInvRoot2 };
	const float kLineRow[8] = { 0.f, 0.f, 1.f, -1.f, kInvRoot2, -kInvRoot2, kInvRoot2, -kInvRoot2 };

	// The far rank, marked across the board.
	const float kRankWidth = 0.18f;
	const float kRankAlpha = 0.45f;
}

TaintedPawnActor::TaintedPawnActor(int region)
	: TaintedPieceActor("잠식된 폰", region, Origin::Dark, Tuning::TaintedPawn::kMaxHealth, Tuning::TaintedPawn::kFuryTime)
{
}

const char* TaintedPawnActor::Hint() const
{
	return m_Promoted ? "승격했다" : "끝 칸에 닿으면 승격한다 · 앞을 막아서라";
}

float TaintedPawnActor::Scale() const
{
	return m_Promoted ? Tuning::TaintedPawn::kPromotedScale : 1.f;
}

float TaintedPawnActor::Windup() const
{
	float windup = m_Sliding ? Tuning::TaintedPawn::kSlideWindup : Tuning::TaintedPawn::kStrikeWindup;
	if (m_Promoted)
	{
		windup *= Tuning::TaintedPawn::kPromotedWindupScale;
	}
	return windup / FurySpeed();
}

void TaintedPawnActor::Square(int& col, int& row) const
{
	float c, r;
	WorldGrid::InverseIso(WorldX(), WorldY(), c, r);
	col = (int)floorf(c + 0.5f);
	row = (int)floorf(r + 0.5f);
}

void TaintedPawnActor::OnUpdate(const ActorContext& ctx)
{
	LookAfterSoldiers();
	TaintedPieceActor::OnUpdate(ctx);
}

void TaintedPawnActor::LookAfterSoldiers()
{
	for (int i = 0; i < Tuning::TaintedPawn::kSoldiers; i++)
	{
		PawnSoldierActor* soldier = m_Soldiers[i];
		if (soldier == nullptr)
		{
			continue;
		}
		// At the far rank it gives itself to its pawn.
		if (soldier->ReachedEnd())
		{
			Mend(Tuning::TaintedPawn::kMaxHealth * Tuning::TaintedPawn::kSoldierHeal);
			soldier->Destroy();
			m_Soldiers[i] = nullptr;
		}
		else if (soldier->IsDead())
		{
			soldier->Destroy();
			m_Soldiers[i] = nullptr;
		}
	}
}

void TaintedPawnActor::SpawnSoldiers(const ActorContext& ctx)
{
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);
	int half = Tuning::TaintedPiece::kBoardHalf;
	int spread = (int)Tuning::TaintedPawn::kSoldierSpread;
	for (int i = 0; i < Tuning::TaintedPawn::kSoldiers; i++)
	{
		int side = (i % 2 == 0) ? -1 : 1;
		std::unique_ptr<PawnSoldierActor> soldier(new PawnSoldierActor(centerCol + side * spread * (i / 2 + 1),
			centerRow - half, centerRow + half));
		m_Soldiers[i] = soldier.get();
		ctx.graph->Spawn(std::move(soldier));
	}
	m_SoldiersOut = true;
}

void TaintedPawnActor::ClearSoldiers()
{
	for (int i = 0; i < Tuning::TaintedPawn::kSoldiers; i++)
	{
		if (m_Soldiers[i] != nullptr)
		{
			m_Soldiers[i]->Destroy();
			m_Soldiers[i] = nullptr;
		}
	}
	m_SoldiersOut = false;
}

void TaintedPawnActor::Promote(const ActorContext& ctx, bool reachedEnd)
{
	m_Promoted = true;
	if (reachedEnd)
	{
		Mend(Tuning::TaintedPawn::kMaxHealth * Tuning::TaintedPawn::kPromoteHeal);
	}
	ctx.impact->Shake(kPromoteShake);
	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Dark, 2.5f, false));
	burst->SetLocalPosition(WorldX(), WorldY());
	ctx.graph->Spawn(std::move(burst));
	SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kPromoteSparks, 220.f, 40.f);
}

void TaintedPawnActor::Fight(const ActorContext& ctx)
{
	if (!m_SoldiersOut)
	{
		SpawnSoldiers(ctx);
	}
	if (!m_Promoted && HealthFraction() < Tuning::TaintedPawn::kPromoteShare && m_State == State::Wait)
	{
		Promote(ctx, false);
	}

	m_StateTime += ctx.deltaTime;
	float dirCol, dirRow;
	float distance = ToPlayer(ctx, dirCol, dirRow);
	int col, row;
	Square(col, row);
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);
	int endRow = centerRow + Tuning::TaintedPiece::kBoardHalf;

	switch (m_State)
	{
	case State::Wait:
	{
		m_Cooldown -= ctx.deltaTime;
		if (m_Cooldown > 0.f)
		{
			break;
		}

		if (m_Promoted)
		{
			// Like a queen: along whichever of the eight lines passes
			// closest to him, through him and a little past.
			float toCol = dirCol * distance;
			float toRow = dirRow * distance;
			int best = 0;
			float bestMiss = 1e9f;
			float bestAlong = 1.f;
			for (int i = 0; i < 8; i++)
			{
				float along = toCol * kLineCol[i] + toRow * kLineRow[i];
				if (along <= 0.5f)
				{
					continue;
				}
				float miss = fabsf(toCol * kLineRow[i] - toRow * kLineCol[i]);
				if (miss < bestMiss)
				{
					bestMiss = miss;
					best = i;
					bestAlong = along;
				}
			}
			float length = bestAlong + 1.f;
			if (length > Tuning::TaintedPawn::kSlideMax)
			{
				length = Tuning::TaintedPawn::kSlideMax;
			}
			WorldGrid::InverseIso(WorldX(), WorldY(), m_FromCol, m_FromRow);
			m_ToCol = m_FromCol + kLineCol[best] * length;
			m_ToRow = m_FromRow + kLineRow[best] * length;
			ClampToBoard(m_ToCol, m_ToRow);
			FaceAlong(kLineCol[best], kLineRow[best]);
			m_Sliding = true;
			m_State = State::Windup;
			m_StateTime = 0.f;
			break;
		}

		// Near enough: it takes on the forward diagonals.
		if (distance <= Tuning::TaintedPawn::kStrikeReach)
		{
			FaceAlong(0.f, 1.f);
			m_Sliding = false;
			m_State = State::Windup;
			m_StateTime = 0.f;
			break;
		}

		// He stands on the square ahead: it cannot step.
		float playerCol, playerRow;
		WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
		float ac = playerCol - col;
		float ar = playerRow - (row + 1);
		if (ac * ac + ar * ar <= Tuning::TaintedPawn::kBlockReach * Tuning::TaintedPawn::kBlockReach || row >= endRow)
		{
			m_Cooldown = Tuning::TaintedPawn::kTurnTime * 0.5f / FurySpeed();
			break;
		}
		m_FromCol = (float)col;
		m_FromRow = (float)row;
		m_ToCol = (float)col;
		m_ToRow = (float)(row + 1);
		FaceAlong(0.f, 1.f);
		m_State = State::Step;
		m_StateTime = 0.f;
		break;
	}
	case State::Step:
	{
		float t = m_StateTime / Tuning::TaintedPawn::kStepTime;
		if (t > 1.f)
		{
			t = 1.f;
		}
		float x, y;
		WorldGrid::IsoPosition(m_FromCol, m_FromRow + (m_ToRow - m_FromRow) * t, x, y);
		SetLocalPosition(x, y);
		if (t >= 1.f)
		{
			m_State = State::Wait;
			m_Cooldown = Tuning::TaintedPawn::kTurnTime / FurySpeed();
			// The far rank: promoted, and mended for having made it.
			if ((int)m_ToRow >= endRow && !m_Promoted)
			{
				Promote(ctx, true);
			}
		}
		break;
	}
	case State::Windup:
	{
		if (m_StateTime < Windup())
		{
			break;
		}
		if (m_Sliding)
		{
			m_State = State::Slide;
			m_StateTime = 0.f;
			m_Landed = false;
			break;
		}
		for (int side = -1; side <= 1; side += 2)
		{
			float x, y;
			WorldGrid::IsoPosition((float)(col + side), (float)(row + 1), x, y);
			Combat::StrikePlayerRadius(ctx, x, y, Tuning::TaintedPawn::kStrikeRadius,
				Tuning::TaintedPawn::kStrikeDamage * FuryDamage());
		}
		ctx.impact->Shake(kStrikeShake);
		m_State = State::Wait;
		m_Cooldown = Tuning::TaintedPawn::kTurnTime * 0.6f / FurySpeed();
		break;
	}
	case State::Slide:
	{
		float t = m_StateTime / (Tuning::TaintedPawn::kSlideTime / FurySpeed());
		if (t > 1.f)
		{
			t = 1.f;
		}
		float x, y;
		WorldGrid::IsoPosition(m_FromCol + (m_ToCol - m_FromCol) * t, m_FromRow + (m_ToRow - m_FromRow) * t, x, y);
		SetLocalPosition(x, y);
		if (!m_Landed && Combat::StrikePlayerRadius(ctx, x, y, Tuning::TaintedPawn::kSlideWidth * 0.5f + BodyRadius() * 0.5f,
			Tuning::TaintedPawn::kSlideDamage * FuryDamage()))
		{
			m_Landed = true;
		}
		if (t >= 1.f)
		{
			ctx.impact->Shake(kStrikeShake);
			m_State = State::Wait;
			m_Cooldown = Tuning::TaintedPawn::kCooldown / FurySpeed();
		}
		break;
	}
	}
}

void TaintedPawnActor::OnReset(const ActorContext& ctx)
{
	// Back on its own square, whole: unpromoted, and its soldiers called back.
	ClearSoldiers();
	m_Promoted = false;
	m_State = State::Wait;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
}

void TaintedPawnActor::OnFall(const ActorContext& ctx)
{
	ClearSoldiers();
}

void TaintedPawnActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);
	int half = Tuning::TaintedPiece::kBoardHalf;

	// The far rank, while it still has somewhere to go.
	if (!m_Promoted && !IsDormant())
	{
		float r, g, b;
		AlignmentColor::Warning(Alignment::Dark, Palette::kWarningWash, r, g, b);
		float startCol = centerCol - half - 0.5f;
		float lineRow = centerRow + half + 0.5f;
		float worldX, worldY, sx, sy;
		WorldGrid::IsoPosition(startCol, lineRow, worldX, worldY);
		ctx.ToScreen(worldX, worldY, sx, sy);
		out.AddGroundStrip(worldX, worldY, sx, sy + WorldGrid::GroundHeightAt(worldX, worldY) + 2.f, 1.f, 0.f, 0.f,
			half * 2.f + 1.f, kRankWidth, r, g, b, kRankAlpha);
	}

	if (m_State != State::Windup)
	{
		return;
	}
	float progress = m_StateTime / Windup();
	if (progress > 1.f)
	{
		progress = 1.f;
	}

	if (m_Sliding)
	{
		float lineCol = m_ToCol - m_FromCol;
		float lineRow = m_ToRow - m_FromRow;
		float length = sqrtf(lineCol * lineCol + lineRow * lineRow);
		if (length > 0.001f)
		{
			MarkStrip(out, ctx, m_FromCol, m_FromRow, lineCol / length, lineRow / length, length,
				Tuning::TaintedPawn::kSlideWidth, progress, Alignment::Dark);
		}
		return;
	}

	int col, row;
	Square(col, row);
	MarkSquare(out, ctx, col - 1, row + 1, 0.15f + 0.3f * progress, Alignment::Dark);
	MarkSquare(out, ctx, col + 1, row + 1, 0.15f + 0.3f * progress, Alignment::Dark);
}

void TaintedPawnActor::DrawHead(DrawList& out, float depth, float x, float base, float top) const
{
	// A neck flaring to a round head; promoted, a crown of points on it.
	Part(out, depth, x, base, top, 6.f, 0.45f, 1.3f, Stone(0));
	Part(out, depth + 0.005f, x, base, top + 6.f, 12.f, 0.6f, 1.15f, Stone(1));
	Part(out, depth + 0.01f, x, base, top + 18.f, 7.f, 0.7f, 0.35f, Stone(0));
	if (!m_Promoted || HasFallen())
	{
		return;
	}
	float scale = Scale();
	float crown = base + (top + 22.f) * scale;
	const Palette::Color& c = Stone(1);
	for (int i = -2; i <= 2; i++)
	{
		out.AddTaperedBox(depth + 0.015f + (i == 0 ? 0.002f : 0.f), x + i * 5.f * scale, crown, 0.08f, 0.08f,
			(i == 0 ? 12.f : 8.f) * scale, 0.2f, i * 1.5f, 0.f, c.r, c.g, c.b);
	}
	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);
	out.AddRect(depth + 0.02f, x, crown + 2.f * scale, 5.f, 3.f, r, g, b, 1.f);
}
