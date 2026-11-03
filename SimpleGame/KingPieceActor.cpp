#include "stdafx.h"
#include "KingPieceActor.h"
#include "QueenPieceActor.h"
#include "ThroneRoomActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "Progress.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const int kSegments = 24;

	const int kSlamSparks = 14;
	const float kSlamSparkSpeed = 200.f;
	const float kSparkLift = 24.f;
	const float kSlamShow = 0.3f;
	const float kSlamRadius = 1.6f; // tiles: every square round his own
}

KingPieceActor::KingPieceActor(int region, bool light, bool still)
	: TaintedPieceActor(light ? "흰 킹 루키우스" : "검은 킹 녹티스", region, light ? Origin::Light : Origin::Dark,
		Tuning::KingPiece::kMaxHealth, Tuning::KingPiece::kFuryTime, still ? -1.f : Tuning::RoyalPiece::kWakeRange,
		Tuning::RoyalPiece::kLeashRange, true)
	, m_Light(light)
	, m_Still(still)
{
}

float KingPieceActor::Scale() const
{
	return Tuning::RoyalPiece::kScale * 1.1f;
}

int KingPieceActor::BoardHalf() const
{
	return ThroneRoomActor::FloorReach();
}

const char* KingPieceActor::Hint() const
{
	if (m_Guarded)
	{
		return "퀸이 킹을 지킨다";
	}
	return m_Light ? "빛의 보호막  ·  어둠폼으로만" : "어둠의 보호막  ·  빛폼으로만";
}

bool KingPieceActor::Shields(Form form) const
{
	return m_Light ? form == Form::Light : form == Form::Dark;
}

float KingPieceActor::Resist(float damage)
{
	if (m_Guarded || Shields(PlayerForm()))
	{
		return 0.f;
	}
	return damage;
}

void KingPieceActor::Fight(const ActorContext& ctx)
{
	if (m_Still)
	{
		return;
	}

	// His queen, while she stands, guards him.
	bool guarded = false;
	ctx.graph->ForEach<QueenPieceActor>([&](QueenPieceActor& queen)
	{
		if (!queen.IsDestroyed() && queen.HealthFraction() > 0.f)
		{
			guarded = true;
		}
	});
	m_Guarded = guarded;

	m_StateTime += ctx.deltaTime;
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_SlamFlash > 0.f)
	{
		m_SlamFlash -= ctx.deltaTime / kSlamShow;
	}
	UpdateCheck(ctx);

	float dirCol, dirRow;
	float distance = ToPlayer(ctx, dirCol, dirRow);

	switch (m_State)
	{
	case State::Wait:
	{
		FaceAlong(dirCol, dirRow);
		if (m_Cooldown <= 0.f)
		{
			Choose(ctx, distance);
		}
		break;
	}
	case State::Step:
	{
		float t = m_StateTime / (Tuning::KingPiece::kStepTime / FurySpeed());
		if (t > 1.f)
		{
			t = 1.f;
		}
		float x, y;
		WorldGrid::IsoPosition(m_FromCol + (m_ToCol - m_FromCol) * t, m_FromRow + (m_ToRow - m_FromRow) * t, x, y);
		SetLocalPosition(x, y);
		if (t >= 1.f)
		{
			Finish();
		}
		break;
	}
	case State::Windup:
	{
		if (m_StateTime >= m_Windup)
		{
			Release(ctx);
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::KingPiece::kCooldown * 0.5f / FurySpeed())
		{
			m_State = State::Wait;
			m_StateTime = 0.f;
		}
		break;
	}
	}
}

void KingPieceActor::Choose(const ActorContext& ctx, float distance)
{
	// Near enough, he slams. Every few moves - more often once worn down -
	// he calls check. Otherwise he steps a square toward him.
	if (distance <= Tuning::KingPiece::kSlamReach)
	{
		Start(Blow::Slam, Tuning::KingPiece::kSlamWindup);
		return;
	}
	int checkEvery = (HealthFraction() < Tuning::KingPiece::kPhaseShare) ? 2 : 3;
	if (m_MoveCount % checkEvery == checkEvery - 1 && m_CheckLeft <= 0.f)
	{
		float col, row;
		WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
		m_CheckCol = (int)floorf(col + 0.5f);
		m_CheckRow = (int)floorf(row + 0.5f);
		Start(Blow::Check, Tuning::KingPiece::kCheckWindup);
		return;
	}

	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float stepCol = (playerCol - col > 0.5f) ? 1.f : ((playerCol - col < -0.5f) ? -1.f : 0.f);
	float stepRow = (playerRow - row > 0.5f) ? 1.f : ((playerRow - row < -0.5f) ? -1.f : 0.f);
	m_FromCol = floorf(col + 0.5f);
	m_FromRow = floorf(row + 0.5f);
	m_ToCol = m_FromCol + stepCol;
	m_ToRow = m_FromRow + stepRow;
	ClampToBoard(m_ToCol, m_ToRow);
	if (WorldGrid::IsBlocked(m_ToCol, m_ToRow))
	{
		// A pillar in the way: he waits a beat instead.
		m_ToCol = m_FromCol;
		m_ToRow = m_FromRow;
	}
	m_State = State::Step;
	m_StateTime = 0.f;
}

void KingPieceActor::Start(Blow blow, float windup)
{
	m_Blow = blow;
	m_Windup = windup / FurySpeed();
	m_State = State::Windup;
	m_StateTime = 0.f;
}

void KingPieceActor::Release(const ActorContext& ctx)
{
	if (m_Blow == Blow::Slam)
	{
		Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(), kSlamRadius, Tuning::KingPiece::kSlamDamage * FuryDamage());
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dust, kSlamSparks, kSlamSparkSpeed, kSparkLift);
		ctx.impact->Shake(Tuning::KingPiece::kSlamShake);
		m_SlamFlash = 1.f;
	}
	else
	{
		m_CheckLeft = Tuning::KingPiece::kCheckTime;
		m_CheckTick = 0.f;
	}
	Finish();
}

void KingPieceActor::UpdateCheck(const ActorContext& ctx)
{
	if (m_CheckLeft <= 0.f)
	{
		return;
	}
	m_CheckLeft -= ctx.deltaTime;
	m_CheckTick -= ctx.deltaTime;
	if (m_CheckTick > 0.f)
	{
		return;
	}
	m_CheckTick = Tuning::KingPiece::kCheckTick;

	// Standing anywhere in the burning row or column, he is struck.
	float playerCol, playerRow;
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	if (InCheck(playerCol, playerRow))
	{
		Combat::StrikePlayerRadius(ctx, ctx.playerX, ctx.playerY, 0.5f, Tuning::KingPiece::kCheckDamage * FuryDamage());
	}
}

bool KingPieceActor::InCheck(float col, float row) const
{
	if (!IsOnBoard(col, row))
	{
		return false;
	}
	return fabsf(col - m_CheckCol) <= 0.5f || fabsf(row - m_CheckRow) <= 0.5f;
}

void KingPieceActor::Finish()
{
	m_Cooldown = Tuning::KingPiece::kCooldown / FurySpeed();
	m_MoveCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void KingPieceActor::OnReset(const ActorContext& ctx)
{
	m_State = State::Wait;
	m_StateTime = 0.f;
	m_Cooldown = 1.5f;
	m_MoveCount = 0;
	m_CheckLeft = 0.f;
}

void KingPieceActor::Reward(const ActorContext& ctx)
{
	ScatterMotes(ctx, Alignment::Light, Tuning::KingPiece::kRewardMotes);
	ScatterMotes(ctx, Alignment::Dark, Tuning::KingPiece::kRewardMotes);
}

void KingPieceActor::OnFall(const ActorContext& ctx)
{
	// Checkmate: the pair is broken, for good. The scene sees it and plays
	// the rest.
	ctx.progress->Set(m_Light ? Story::LightPairBroken : Story::DarkPairBroken);
	m_CheckLeft = 0.f;
}

void KingPieceActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
	Alignment color = m_Light ? Alignment::Light : Alignment::Dark;
	int half = BoardHalf();
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);

	// The check: gathering, then burning along its row and column.
	bool gathering = (m_State == State::Windup && m_Blow == Blow::Check);
	if (gathering || m_CheckLeft > 0.f)
	{
		float progress = gathering ? ((m_Windup > 0.f) ? m_StateTime / m_Windup : 1.f) : 1.f;
		if (progress > 1.f)
		{
			progress = 1.f;
		}
		float pulse = gathering ? 1.f : 0.75f + 0.25f * sinf(Time() * 10.f);
		MarkStrip(out, ctx, (float)(centerCol - half), (float)m_CheckRow, 1.f, 0.f, (float)(half * 2), 1.f,
			progress * pulse, color);
		MarkStrip(out, ctx, (float)m_CheckCol, (float)(centerRow - half), 0.f, 1.f, (float)(half * 2), 1.f,
			progress * pulse, color);
	}

	// The slam: every square round his.
	if (m_State == State::Windup && m_Blow == Blow::Slam)
	{
		float progress = (m_Windup > 0.f) ? m_StateTime / m_Windup : 1.f;
		float col, row;
		WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
		int baseCol = (int)floorf(col + 0.5f);
		int baseRow = (int)floorf(row + 0.5f);
		for (int dr = -1; dr <= 1; dr++)
		{
			for (int dc = -1; dc <= 1; dc++)
			{
				MarkSquare(out, ctx, baseCol + dc, baseRow + dr, 0.2f + 0.5f * (progress > 1.f ? 1.f : progress), color);
			}
		}
	}
}

void KingPieceActor::DrawHead(DrawList& out, float depth, float x, float base, float top) const
{
	// A tall crown and a cross over it.
	Part(out, depth, x, base, top, 12.f, 0.7f, 1.25f, Stone(0));
	Part(out, depth + 0.005f, x, base, top + 12.f, 3.f, 0.8f, 1.f, Stone(1));
	Part(out, depth + 0.01f, x, base, top + 15.f, 12.f, 0.12f, 1.f, Stone(0));
	Part(out, depth + 0.012f, x, base, top + 21.f, 3.f, 0.42f, 1.f, Stone(0));
}

void KingPieceActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	TaintedPieceActor::OnDraw(out, ctx);
	if (HasFallen() || m_Still)
	{
		return;
	}

	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 200.f, 200.f))
	{
		return;
	}
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;
	float pulse = 0.85f + 0.15f * sinf(Time() * 4.f);

	// His nation's ward, a ring round his foot; and while his queen stands,
	// her guard over him, a dome of her colour.
	float r, g, b;
	AlignmentColor::Core(m_Light ? Alignment::Light : Alignment::Dark, r, g, b);
	out.SetGlow(m_Light ? 0.7f : 0.2f);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + 2.f, 1.3f, 1.45f, 0.f, 2.f * kPi, kSegments, r, g, b, 0.7f * pulse);
	if (m_Guarded)
	{
		out.AddGroundArc(WorldX(), WorldY(), x, groundY + 40.f, 1.1f, 1.2f, 0.f, 2.f * kPi, kSegments, r, g, b, 0.45f * pulse);
		out.AddGroundArc(WorldX(), WorldY(), x, groundY + 80.f, 0.6f, 0.7f, 0.f, 2.f * kPi, kSegments, r, g, b, 0.35f * pulse);
		out.AddGlow(depth + 0.2f, x, groundY + 50.f, 90.f, r, g, b, 0.3f * pulse);
	}
	out.SetGlow(0.f);
}
