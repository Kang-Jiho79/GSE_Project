#include "stdafx.h"
#include "TaintedPieceActor.h"
#include "Bosses.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Impact.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;

	// Its stone: the white and black of the board, gone this far to gray.
	const Palette::Color kWhiteStone = { 0.70f, 0.70f, 0.72f };
	const Palette::Color kBlackStone = { 0.15f, 0.14f, 0.17f };
	const Palette::Color kTaint = { 0.40f, 0.40f, 0.42f };
	// A piece the gray has not touched: clean white and black.
	const Palette::Color kWholeWhite = { 0.88f, 0.87f, 0.84f };
	const Palette::Color kWholeBlack = { 0.09f, 0.08f, 0.11f };
	const float kTaintShare = 0.35f;

	// The cracks spreading up it as it is hurt: a few from the start, the
	// rest as its life goes. Pixels up its length, and across.
	const int kCracks = 8;
	const int kFirstCracks = 2;
	const float kCrackUp[kCracks] = { 18.f, 30.f, 12.f, 24.f, 36.f, 8.f, 28.f, 40.f };
	const float kCrackAcross[kCracks] = { -4.f, 5.f, 6.f, -6.f, 2.f, -2.f, -1.f, 4.f };
	const float kCrackLength[kCracks] = { 10.f, 8.f, 7.f, 9.f, 6.f, 5.f, 8.f, 5.f };

	// Falling: over this far, then rolling this far along the way it fell.
	const float kToppleShare = 0.45f;      // of the fall spent going over
	const float kToppleAngle = 1.25f;      // radians, short of lying flat
	const float kRollDistance = 26.f;      // pixels
	const int kBreakDust = 22;
	const float kBreakDustSpeed = 160.f;
	const float kBreakDustLift = 20.f;

	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;

	Palette::Color Tainted(const Palette::Color& c)
	{
		Palette::Color out =
		{
			c.r + (kTaint.r - c.r) * kTaintShare,
			c.g + (kTaint.g - c.g) * kTaintShare,
			c.b + (kTaint.b - c.b) * kTaintShare,
		};
		return out;
	}
}

TaintedPieceActor::TaintedPieceActor(const char* name, int region, Origin origin, float maxHealth, float furyTime)
	: TaintedPieceActor(name, region, origin, maxHealth, furyTime, Tuning::TaintedPiece::kWakeRange,
		Tuning::TaintedPiece::kLeashRange, false)
{
}

TaintedPieceActor::TaintedPieceActor(const char* name, int region, Origin origin, float maxHealth, float furyTime,
	float wakeRange, float leashRange, bool whole)
	: BossActor(name, maxHealth, region, wakeRange, leashRange, furyTime)
	, m_Region(region)
	, m_Origin(origin)
	, m_Whole(whole)
{
}

int TaintedPieceActor::BoardHalf() const
{
	return Tuning::TaintedPiece::kBoardHalf;
}

float TaintedPieceActor::BodyRadius() const
{
	return Tuning::TaintedPiece::kBodyRadius * Scale();
}

TaintedPieceActor::Origin TaintedPieceActor::GetOrigin() const
{
	return m_Origin;
}

bool TaintedPieceActor::HasFallen() const
{
	return m_Fallen;
}

float TaintedPieceActor::Scale() const
{
	return 1.f;
}

float TaintedPieceActor::Lift() const
{
	return 0.f;
}

void TaintedPieceActor::OnFall(const ActorContext& ctx)
{
}

void TaintedPieceActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
}

void TaintedPieceActor::Reward(const ActorContext& ctx)
{
	int motes = Tuning::TaintedPiece::kRewardMotes;
	switch (m_Origin)
	{
	case Origin::Light:
	{
		ScatterMotes(ctx, Alignment::Light, motes);
		break;
	}
	case Origin::Dark:
	{
		ScatterMotes(ctx, Alignment::Dark, motes);
		break;
	}
	default:
	{
		ScatterMotes(ctx, Alignment::Light, motes / 2);
		ScatterMotes(ctx, Alignment::Dark, motes / 2);
		break;
	}
	}
}

void TaintedPieceActor::Defeated(const ActorContext& ctx)
{
	if (!m_Fallen)
	{
		m_Fallen = true;
		m_FallTime = 0.f;
		m_FallFacing = Facing();
		OnFall(ctx);
		ctx.impact->HitStop(Tuning::Boss::kDefeatStop);
		return;
	}

	m_FallTime += ctx.deltaTime;
	if (m_FallTime < Tuning::TaintedPiece::kToppleTime)
	{
		return;
	}

	// Rolled to a stop, it breaks apart; what it was is left for him.
	float rollX = m_FallFacing * kRollDistance;
	SparkActor::Burst(*ctx.graph, WorldX() + rollX, WorldY(), SparkActor::Style::Dust,
		kBreakDust, kBreakDustSpeed, kBreakDustLift);
	BossActor::Defeated(ctx);
}

void TaintedPieceActor::Part(DrawList& out, float depth, float x, float base, float from, float height, float foot,
	float taper, const Palette::Color& color) const
{
	float scale = Scale();
	from *= scale;
	height *= scale;
	foot *= scale;

	// Falling: tipped over the way it faced, then rolling on along it.
	float angle = 0.f;
	float roll = 0.f;
	float facing = m_Fallen ? m_FallFacing : Facing();
	if (m_Fallen)
	{
		float t = m_FallTime / Tuning::TaintedPiece::kToppleTime;
		float over = (t < kToppleShare) ? t / kToppleShare : 1.f;
		angle = kToppleAngle * over * over;
		float rolling = (t > kToppleShare) ? (t - kToppleShare) / (1.f - kToppleShare) : 0.f;
		roll = kRollDistance * rolling;
	}
	float up = cosf(angle);
	float side = sinf(angle) * facing;
	out.AddTaperedBox(depth, x + side * from + facing * roll, base + up * from, foot, foot, height * up, taper,
		side * height, 0.f, color.r, color.g, color.b);
}

Palette::Color TaintedPieceActor::Stone(int band) const
{
	bool white = (m_Origin == Origin::Light) || (m_Origin == Origin::Mixed && band % 2 == 0);
	if (m_Whole)
	{
		return white ? kWholeWhite : kWholeBlack;
	}
	return Tainted(white ? kWhiteStone : kBlackStone);
}

Alignment TaintedPieceActor::CrackColor(int index) const
{
	if (m_Origin == Origin::Light)
	{
		return Alignment::Light;
	}
	if (m_Origin == Origin::Dark)
	{
		return Alignment::Dark;
	}
	return (index % 2 == 0) ? Alignment::Light : Alignment::Dark;
}

void TaintedPieceActor::BoardCenter(int& col, int& row) const
{
	Bosses::BoardCenter(m_Region, col, row);
}

bool TaintedPieceActor::IsPaleSquare(int col, int row) const
{
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);
	return ((col - centerCol + row - centerRow) % 2 + 2) % 2 == 0;
}

bool TaintedPieceActor::IsOnBoard(float col, float row) const
{
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);
	float half = BoardHalf() + 0.5f;
	return fabsf(col - centerCol) <= half && fabsf(row - centerRow) <= half;
}

void TaintedPieceActor::ClampToBoard(float& col, float& row) const
{
	int centerCol, centerRow;
	BoardCenter(centerCol, centerRow);
	float half = (float)BoardHalf();
	if (col < centerCol - half)
	{
		col = centerCol - half;
	}
	if (col > centerCol + half)
	{
		col = centerCol + half;
	}
	if (row < centerRow - half)
	{
		row = centerRow - half;
	}
	if (row > centerRow + half)
	{
		row = centerRow + half;
	}
}

void TaintedPieceActor::MarkSquare(DrawList& out, const ActorContext& ctx, int col, int row, float alpha,
	Alignment color) const
{
	float r, g, b;
	AlignmentColor::Warning(color, Palette::kWarningWash, r, g, b);
	float worldX, worldY, x, y;
	WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	if (!ctx.IsOnScreen(x, y, WorldGrid::kTileWidth, WorldGrid::kTileHeight))
	{
		return;
	}
	out.AddDiamond((float)(col + row) + 0.03f, x, y + WorldGrid::GroundHeightAt(worldX, worldY),
		WorldGrid::kTileWidth - 6.f, WorldGrid::kTileHeight - 3.f, r, g, b, alpha);
}

void TaintedPieceActor::MarkStrip(DrawList& out, const ActorContext& ctx, float fromCol, float fromRow,
	float dirCol, float dirRow, float length, float width, float progress, Alignment color) const
{
	float r, g, b;
	AlignmentColor::Warning(color, Palette::kWarningWash, r, g, b);
	float worldX, worldY, x, y;
	WorldGrid::IsoPosition(fromCol, fromRow, worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	y += WorldGrid::GroundHeightAt(worldX, worldY) + kTelegraphLift;
	out.AddGroundStrip(worldX, worldY, x, y, dirCol, dirRow, 0.f, length, width, r, g, b, kTelegraphBase);
	if (progress > 0.f)
	{
		out.AddGroundStrip(worldX, worldY, x, y, dirCol, dirRow, 0.f, length * progress, width, r, g, b, kTelegraphFill);
	}
}

void TaintedPieceActor::MarkCircle(DrawList& out, const ActorContext& ctx, float col, float row, float radius,
	float progress, Alignment color) const
{
	float r, g, b;
	AlignmentColor::Warning(color, Palette::kWarningWash, r, g, b);
	float worldX, worldY, x, y;
	WorldGrid::IsoPosition(col, row, worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	y += WorldGrid::GroundHeightAt(worldX, worldY) + kTelegraphLift;
	out.AddGroundArc(worldX, worldY, x, y, 0.f, radius, 0.f, 2.f * kPi, kTelegraphSegments, r, g, b, kTelegraphBase);
	if (progress > 0.f)
	{
		out.AddGroundArc(worldX, worldY, x, y, 0.f, radius * progress, 0.f, 2.f * kPi, kTelegraphSegments,
			r, g, b, kTelegraphFill);
	}
	out.AddGroundArc(worldX, worldY, x, y, radius - kTelegraphEdgeWidth, radius, 0.f, 2.f * kPi, kTelegraphSegments,
		r, g, b, kTelegraphEdge);
}

void TaintedPieceActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 300.f, 300.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;
	float scale = Scale();

	if (!m_Fallen)
	{
		DrawTelegraph(out, ctx, x, groundY);
	}

	float lift = Lift();
	float shadowScale = scale * (1.f - lift / 120.f);
	out.AddShadow(depth - 0.01f, x, groundY, 70.f * shadowScale, 35.f * shadowScale);
	DrawFury(out, depth - 0.005f, x, groundY, 70.f * scale);

	out.SetFigure(true);
	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.75f);
	}

	// A stepped pedestal, a column narrowing as it rises, and a collar.
	float base = groundY + lift;
	Part(out, depth, x, base, 0.f, 6.f, 1.4f, 0.9f, Stone(0));
	Part(out, depth + 0.005f, x, base, 6.f, 5.f, 1.15f, 0.85f, Stone(1));
	Part(out, depth + 0.01f, x, base, 11.f, 30.f, 0.75f, 0.75f, Stone(0));
	Part(out, depth + 0.015f, x, base, 41.f, 4.f, 0.95f, 1.f, Stone(1));
	DrawHead(out, depth + 0.02f, x, base, 45.f);
	out.SetFigure(false);
	out.SetFlash(0.f);

	// The cracks, more of them the more it has been hurt; what it was
	// shows through - light shines, dark does not. A whole piece has none.
	if (!m_Fallen && !m_Whole)
	{
		int cracks = kFirstCracks + (int)((1.f - HealthFraction()) * (kCracks - kFirstCracks) + 0.5f);
		float pulse = IsDormant() ? 0.55f : 0.8f + 0.2f * sinf(Time() * 4.f);
		for (int i = 0; i < cracks && i < kCracks; i++)
		{
			Alignment alignment = CrackColor(i);
			float r, g, b;
			AlignmentColor::Core(alignment, r, g, b);
			out.SetGlow(alignment == Alignment::Light ? 0.8f * pulse : 0.f);
			out.AddRect(depth + 0.03f, x + kCrackAcross[i] * scale, base + kCrackUp[i] * scale, 1.6f,
				kCrackLength[i] * scale, r * pulse, g * pulse, b * pulse, 1.f);
		}
		out.SetGlow(0.f);
	}
}
