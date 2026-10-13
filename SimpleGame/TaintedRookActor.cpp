#include "stdafx.h"
#include "TaintedRookActor.h"
#include "ArenaStoneActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>
#include <cstdlib>

namespace
{
	const float kPi = 3.14159265f;
	const int kStunSparks = 3;
	const float kStunCircle = 0.5f; // tiles, the dizzy ring over it
	const int kChargeDust = 3;
}

TaintedRookActor::TaintedRookActor(int region)
	: TaintedPieceActor("잠식된 룩", region, Origin::Mixed, Tuning::TaintedRook::kMaxHealth, Tuning::TaintedRook::kFuryTime)
{
}

const char* TaintedRookActor::Hint() const
{
	if (m_State == State::Stunned)
	{
		return "기절 · 받는 피해 증가";
	}
	return (PillarsLeft() > 0) ? "기둥 뒤에서 돌진을 유도하라" : nullptr;
}

int TaintedRookActor::PillarsLeft() const
{
	int left = 0;
	for (int i = 0; i < kPillars; i++)
	{
		if (m_Pillar[i] != nullptr && !m_Pillar[i]->IsBroken())
		{
			left++;
		}
	}
	return left;
}

float TaintedRookActor::Resist(float damage)
{
	if (m_State == State::Stunned)
	{
		return damage * Tuning::TaintedRook::kStunScale;
	}
	return damage;
}

void TaintedRookActor::OnUpdate(const ActorContext& ctx)
{
	// Its pillars stand on its board from the start, before it ever wakes.
	if (!m_PillarsRaised)
	{
		RaisePillars(ctx);
	}

	// Walls that have crumbled away are cleared.
	for (int i = 0; i < kMostWalls; i++)
	{
		if (m_Wall[i] != nullptr && m_Wall[i]->IsGone())
		{
			m_Wall[i]->Destroy();
			m_Wall[i] = nullptr;
		}
	}

	TaintedPieceActor::OnUpdate(ctx);
}

void TaintedRookActor::RaisePillars(const ActorContext& ctx)
{
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);
	int offset = (int)Tuning::TaintedRook::kPillarOffset;
	for (int i = 0; i < kPillars; i++)
	{
		int col = centerCol + ((i % 2 == 0) ? -offset : offset);
		int row = centerRow + ((i < 2) ? -offset : offset);
		std::unique_ptr<ArenaStoneActor> pillar(new ArenaStoneActor(col, row, true, 0.f));
		m_Pillar[i] = pillar.get();
		ctx.graph->Spawn(std::move(pillar));
	}
	m_PillarsRaised = true;
}

void TaintedRookActor::ClearWalls()
{
	for (int i = 0; i < kMostWalls; i++)
	{
		if (m_Wall[i] != nullptr)
		{
			m_Wall[i]->Break();
			m_Wall[i]->Destroy();
			m_Wall[i] = nullptr;
		}
	}
}

void TaintedRookActor::AxisToward(const ActorContext& ctx, int& dirCol, int& dirRow) const
{
	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float dc = playerCol - col;
	float dr = playerRow - row;
	dirCol = 0;
	dirRow = 0;
	if (fabsf(dc) >= fabsf(dr))
	{
		dirCol = (dc >= 0.f) ? 1 : -1;
	}
	else
	{
		dirRow = (dr >= 0.f) ? 1 : -1;
	}
}

void TaintedRookActor::Fight(const ActorContext& ctx)
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
		float windup = (m_Blow == Blow::Wall) ? Tuning::TaintedRook::kWallWindup : Tuning::TaintedRook::kChargeWindup;
		if (m_StateTime < windup / FurySpeed())
		{
			break;
		}
		if (m_Blow == Blow::Wall)
		{
			RaiseWall(ctx);
			m_MoveCount++;
			// Straight into a charge, while he is penned.
			m_State = State::Wait;
			m_StateTime = 0.f;
			m_Cooldown = 0.f;
			break;
		}
		m_State = State::Charge;
		m_StateTime = 0.f;
		m_Landed = false;
		break;
	}
	case State::Charge:
	{
		UpdateCharge(ctx);
		break;
	}
	case State::Stunned:
	{
		if (m_StateTime >= Tuning::TaintedRook::kStunTime)
		{
			m_State = State::Recover;
			m_StateTime = 0.f;
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::TaintedRook::kRecover / FurySpeed())
		{
			m_State = State::Wait;
			m_StateTime = 0.f;
			m_Cooldown = Tuning::TaintedRook::kCooldown / FurySpeed();
		}
		break;
	}
	}
}

void TaintedRookActor::Choose(const ActorContext& ctx)
{
	int every = Tuning::TaintedRook::kWallEvery;
	int dirCol, dirRow;
	AxisToward(ctx, dirCol, dirRow);

	if (m_MoveCount % every == every - 1)
	{
		// A wall across the line, just past him, so the next charge pins him.
		m_Blow = Blow::Wall;
		int playerCol, playerRow;
		WorldGrid::WorldToTile(ctx.playerX, ctx.playerY, playerCol, playerRow);
		int behind = (int)(Tuning::TaintedRook::kWallBehind + 0.5f);
		m_WallCol = playerCol + dirCol * behind;
		m_WallRow = playerRow + dirRow * behind;
		m_WallAlongCol = dirRow;
		m_WallAlongRow = dirCol;
		m_State = State::Windup;
		m_StateTime = 0.f;
		return;
	}

	// Along the board's line toward him, as far as the edge of its board.
	m_Blow = Blow::Charge;
	m_DirCol = dirCol;
	m_DirRow = dirRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), m_FromCol, m_FromRow);
	m_Length = Tuning::TaintedRook::kChargeMax;
	float endCol = m_FromCol + dirCol * m_Length;
	float endRow = m_FromRow + dirRow * m_Length;
	ClampToBoard(endCol, endRow);
	m_Length = fabsf(endCol - m_FromCol) + fabsf(endRow - m_FromRow);
	FaceAlong((float)dirCol, (float)dirRow);
	m_State = State::Windup;
	m_StateTime = 0.f;
}

void TaintedRookActor::RaiseWall(const ActorContext& ctx)
{
	int half = Tuning::TaintedRook::kWallLength / 2;
	for (int i = -half; i <= half; i++)
	{
		int col = m_WallCol + m_WallAlongCol * i;
		int row = m_WallRow + m_WallAlongRow * i;
		if (!IsOnBoard((float)col, (float)row))
		{
			continue;
		}
		// Not on top of a pillar, nor of a wall already there.
		bool taken = false;
		for (int p = 0; p < kPillars; p++)
		{
			if (m_Pillar[p] != nullptr && m_Pillar[p]->Col() == col && m_Pillar[p]->Row() == row)
			{
				taken = true;
			}
		}
		int slot = -1;
		for (int w = 0; w < kMostWalls; w++)
		{
			if (m_Wall[w] == nullptr)
			{
				if (slot < 0)
				{
					slot = w;
				}
			}
			else if (m_Wall[w]->Col() == col && m_Wall[w]->Row() == row && !m_Wall[w]->IsBroken())
			{
				taken = true;
			}
		}
		if (taken || slot < 0)
		{
			continue;
		}
		std::unique_ptr<ArenaStoneActor> wall(new ArenaStoneActor(col, row, false, Tuning::TaintedRook::kWallTime));
		m_Wall[slot] = wall.get();
		ctx.graph->Spawn(std::move(wall));
	}
	ctx.impact->Shake(Tuning::TaintedRook::kChargeShake * 0.5f);
}

void TaintedRookActor::UpdateCharge(const ActorContext& ctx)
{
	float travelled = m_StateTime * Tuning::TaintedRook::kChargeSpeed * FurySpeed();
	bool done = false;
	if (travelled >= m_Length)
	{
		travelled = m_Length;
		done = true;
	}
	float col = m_FromCol + m_DirCol * travelled;
	float row = m_FromRow + m_DirRow * travelled;

	// Whatever stone it runs into stops it: a pillar breaks and leaves it
	// stunned; one of its walls only crumbles.
	float reach = Tuning::TaintedRook::kPillarRadius + BodyRadius() * 0.6f;
	for (int i = 0; i < kPillars; i++)
	{
		ArenaStoneActor* pillar = m_Pillar[i];
		if (pillar == nullptr || pillar->IsBroken())
		{
			continue;
		}
		float dc = pillar->Col() - col;
		float dr = pillar->Row() - row;
		if (dc * dc + dr * dr <= reach * reach)
		{
			pillar->Break();
			ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
			ctx.impact->Shake(Tuning::TaintedRook::kChargeShake);
			SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dust, 16, 180.f, 40.f);
			m_MoveCount++;
			m_State = State::Stunned;
			m_StateTime = 0.f;
			return;
		}
	}
	for (int i = 0; i < kMostWalls; i++)
	{
		ArenaStoneActor* wall = m_Wall[i];
		if (wall == nullptr || wall->IsBroken())
		{
			continue;
		}
		float dc = wall->Col() - col;
		float dr = wall->Row() - row;
		if (dc * dc + dr * dr <= reach * reach)
		{
			wall->Break();
			done = true;
		}
	}

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);
	if (!m_Landed && Combat::StrikePlayerRadius(ctx, x, y, Tuning::TaintedRook::kChargeWidth * 0.5f + BodyRadius() * 0.5f,
		Tuning::TaintedRook::kChargeDamage * FuryDamage()))
	{
		m_Landed = true;
	}
	SparkActor::Burst(*ctx.graph, x, y, SparkActor::Style::Dust, kChargeDust, 60.f, 2.f);

	if (done)
	{
		ctx.impact->Shake(Tuning::TaintedRook::kChargeShake);
		m_MoveCount++;
		m_State = State::Recover;
		m_StateTime = 0.f;
	}
}

void TaintedRookActor::OnReset(const ActorContext& ctx)
{
	// Back on its square: its walls gone and its pillars standing again.
	ClearWalls();
	for (int i = 0; i < kPillars; i++)
	{
		if (m_Pillar[i] != nullptr)
		{
			m_Pillar[i]->Restore();
		}
	}
	m_State = State::Wait;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
	m_MoveCount = 0;
}

void TaintedRookActor::OnFall(const ActorContext& ctx)
{
	ClearWalls();
}

void TaintedRookActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
	// Stunned: a ring of dust turning over its head.
	if (m_State == State::Stunned)
	{
		float r, g, b;
		AlignmentColor::Core(Alignment::Ashen, r, g, b);
		float headY = groundY + 80.f;
		for (int i = 0; i < kStunSparks; i++)
		{
			float angle = Time() * 5.f + i * 2.f * kPi / kStunSparks;
			out.AddDiamond(WorldGrid::TileDepth(WorldX(), WorldY()) + 0.7f, x + cosf(angle) * 18.f,
				headY + sinf(angle) * 6.f, 6.f, 6.f, r, g, b, 1.f);
		}
		return;
	}
	if (m_State != State::Windup)
	{
		return;
	}
	float windup = (m_Blow == Blow::Wall) ? Tuning::TaintedRook::kWallWindup : Tuning::TaintedRook::kChargeWindup;
	float progress = m_StateTime / (windup / FurySpeed());
	if (progress > 1.f)
	{
		progress = 1.f;
	}

	if (m_Blow == Blow::Wall)
	{
		int half = Tuning::TaintedRook::kWallLength / 2;
		for (int i = -half; i <= half; i++)
		{
			int col = m_WallCol + m_WallAlongCol * i;
			int row = m_WallRow + m_WallAlongRow * i;
			if (IsOnBoard((float)col, (float)row))
			{
				MarkSquare(out, ctx, col, row, 0.15f + 0.35f * progress, Alignment::Ashen);
			}
		}
		return;
	}
	MarkStrip(out, ctx, m_FromCol, m_FromRow, (float)m_DirCol, (float)m_DirRow, m_Length,
		Tuning::TaintedRook::kChargeWidth, progress, (m_MoveCount % 2 == 0) ? Alignment::Light : Alignment::Dark);
}

void TaintedRookActor::DrawHead(DrawList& out, float depth, float x, float base, float top) const
{
	// A battlemented crown, white and black.
	Part(out, depth, x, base, top, 12.f, 0.9f, 1.08f, Stone(0));
	if (HasFallen())
	{
		return;
	}
	float scale = Scale();
	float reach = 0.97f * WorldGrid::kTileWidth / 2.f;
	float rise = 0.97f * WorldGrid::kTileHeight / 2.f;
	float crown = base + (top + 12.f) * scale;
	const Palette::Color& white = Stone(0);
	const Palette::Color& black = Stone(1);
	out.AddBox(depth + 0.005f, x, crown + rise, 0.22f, 0.22f, 6.f, black.r, black.g, black.b);
	out.AddBox(depth + 0.01f, x - reach, crown, 0.22f, 0.22f, 6.f, white.r, white.g, white.b);
	out.AddBox(depth + 0.01f, x + reach, crown, 0.22f, 0.22f, 6.f, black.r, black.g, black.b);
	out.AddBox(depth + 0.015f, x, crown - rise, 0.22f, 0.22f, 6.f, white.r, white.g, white.b);
}
