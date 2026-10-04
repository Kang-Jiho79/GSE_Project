#include "stdafx.h"
#include "GodPieceActor.h"
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
	const float kPi = 3.14159265f;

	// Pale, cold stone: it belongs to neither side.
	const Palette::Color kStone = { 0.50f, 0.50f, 0.54f };

	const float kLeapHeight = 46.f;   // pixels at the top of a leap
	const float kLandShake = 5.f;
	const float kWardFlickerRate = 0.12f;

	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.12f;
	const float kTelegraphFill = 0.24f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;

	// The eight lines a piece may slide along: the grid's own, and its diagonals.
	const float kInvRoot2 = 0.70710678f;
	const float kLineCol[8] = { 1.f, -1.f, 0.f, 0.f, kInvRoot2, kInvRoot2, -kInvRoot2, -kInvRoot2 };
	const float kLineRow[8] = { 0.f, 0.f, 1.f, -1.f, kInvRoot2, -kInvRoot2, kInvRoot2, -kInvRoot2 };

	// The eight landings of a knight's move, in squares.
	const float kKnightCol[8] = { 2.f, 1.f, -1.f, -2.f, -2.f, -1.f, 1.f, 2.f };
	const float kKnightRow[8] = { 1.f, 2.f, 2.f, 1.f, -1.f, -2.f, -2.f, -1.f };

	// What it marks the ground in belongs to neither side - the god's own
	// pale colour.
	const Palette::Color& MarkColor()
	{
		return Palette::kUiBone;
	}
}

GodPieceActor::GodPieceActor(int region, Kind kind, float maxHealth)
	: BossActor("신의 말", maxHealth, region, Tuning::GodPiece::kWakeRange, Tuning::GodPiece::kLeashRange,
		Tuning::GodPiece::kFuryTime)
	, m_Kind(kind)
{
	m_Ward = (Random::Unit() < 0.5f) ? Alignment::Light : Alignment::Dark;
	m_WardTimer = Tuning::GodPiece::kWardTime;
}

float GodPieceActor::BodyRadius() const
{
	return Tuning::GodPiece::kBodyRadius;
}

float GodPieceActor::Resist(float damage)
{
	// A gold ward turns aside the light, a violet one the dark. The gray is
	// neither, and goes through both.
	if (PlayerForm() == Form::Ash)
	{
		return damage;
	}
	bool light = (PlayerForm() == Form::Light);
	if ((m_Ward == Alignment::Light && light) || (m_Ward == Alignment::Dark && !light))
	{
		return 0.f;
	}
	return damage;
}

void GodPieceActor::UpdateWard(float deltaTime)
{
	m_WardTimer -= deltaTime * FurySpeed();
	if (m_WardTimer <= 0.f)
	{
		m_Ward = (m_Ward == Alignment::Light) ? Alignment::Dark : Alignment::Light;
		m_WardTimer = Tuning::GodPiece::kWardTime;
	}
}

void GodPieceActor::Fight(const ActorContext& ctx)
{
	UpdateWard(ctx.deltaTime);

	m_StateTime += ctx.deltaTime;
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}

	float dirCol, dirRow;
	float distance = ToPlayer(ctx, dirCol, dirRow);

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
			// Strike the colour of square he is standing on.
			int playerCol, playerRow;
			WorldGrid::WorldToTile(ctx.playerX, ctx.playerY, playerCol, playerRow);
			WorldGrid::WorldToTile(WorldX(), WorldY(), m_BoardCol, m_BoardRow);
			m_BoardParity = (playerCol + playerRow) & 1;
			Start(Blow::Board, Tuning::GodPiece::kBoardWindup / FurySpeed());
		}
		else if (m_Kind == Kind::Rook)
		{
			Start(Blow::Slide, Tuning::GodPiece::kSlideWindup / FurySpeed());
			ChooseSlide(dirCol * distance, dirRow * distance);
		}
		else
		{
			m_LeapsLeft = Tuning::GodPiece::kLeapChain;
			Start(Blow::Leap, Tuning::GodPiece::kLeapWindup / FurySpeed());
			ChooseLeap(dirCol * distance, dirRow * distance);
		}
		break;
	}
	case State::Windup:
	{
		if (m_StateTime < m_Windup)
		{
			break;
		}

		if (m_Blow == Blow::Board)
		{
			StrikeBoard(ctx);
			Finish();
		}
		else
		{
			m_State = State::Move;
			m_StateTime = 0.f;
			m_Landed = false;
		}
		break;
	}
	case State::Move:
	{
		UpdateMove(ctx);
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::GodPiece::kRecover / FurySpeed())
		{
			m_State = State::Wait;
			m_StateTime = 0.f;
		}
		break;
	}
	}
}

void GodPieceActor::Start(Blow blow, float windup)
{
	m_Blow = blow;
	m_Windup = windup;
	m_State = State::Windup;
	m_StateTime = 0.f;
	WorldGrid::InverseIso(LocalX(), LocalY(), m_FromCol, m_FromRow);
	m_ToCol = m_FromCol;
	m_ToRow = m_FromRow;
}

void GodPieceActor::ChooseSlide(float toCol, float toRow)
{
	// The line that passes closest to him, of the eight it may take.
	int best = (toCol >= 0.f) ? 0 : 1;
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

	// Through him and a little past, but never further than a piece may go.
	float length = bestAlong + 1.f;
	if (length > Tuning::GodPiece::kSlideMax)
	{
		length = Tuning::GodPiece::kSlideMax;
	}

	m_ToCol = m_FromCol + kLineCol[best] * length;
	m_ToRow = m_FromRow + kLineRow[best] * length;
	WorldGrid::ClampToMap(m_ToCol, m_ToRow);
	FaceAlong(kLineCol[best], kLineRow[best]);
}

void GodPieceActor::ChooseLeap(float toCol, float toRow)
{
	// The one of its eight landings that comes down nearest him.
	float bestDistance = 1e9f;
	for (int i = 0; i < 8; i++)
	{
		float landCol = kKnightCol[i] * Tuning::GodPiece::kLeapScale;
		float landRow = kKnightRow[i] * Tuning::GodPiece::kLeapScale;
		float dc = toCol - landCol;
		float dr = toRow - landRow;
		float distance = dc * dc + dr * dr;
		if (distance < bestDistance)
		{
			bestDistance = distance;
			m_ToCol = m_FromCol + landCol;
			m_ToRow = m_FromRow + landRow;
		}
	}
	WorldGrid::ClampToMap(m_ToCol, m_ToRow);
	FaceAlong(m_ToCol - m_FromCol, m_ToRow - m_FromRow);
}

void GodPieceActor::UpdateMove(const ActorContext& ctx)
{
	float duration = ((m_Blow == Blow::Slide) ? Tuning::GodPiece::kSlideTime : Tuning::GodPiece::kLeapTime) / FurySpeed();
	float t = m_StateTime / duration;
	if (t > 1.f)
	{
		t = 1.f;
	}

	float col = m_FromCol + (m_ToCol - m_FromCol) * t;
	float row = m_FromRow + (m_ToRow - m_FromRow) * t;
	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);

	if (m_Blow == Blow::Slide)
	{
		// Anything in its path as it passes is struck.
		if (!m_Landed && Combat::StrikePlayerRadius(ctx, x, y, Tuning::GodPiece::kSlideWidth * 0.5f + BodyRadius() * 0.5f,
			Tuning::GodPiece::kSlideDamage * FuryDamage()))
		{
			m_Landed = true;
		}
		if (t >= 1.f)
		{
			Finish();
		}
		return;
	}

	m_Lift = sinf(kPi * t) * kLeapHeight;
	if (t < 1.f)
	{
		return;
	}

	m_Lift = 0.f;
	Combat::StrikePlayerRadius(ctx, x, y, Tuning::GodPiece::kLandRadius, Tuning::GodPiece::kLeapDamage * FuryDamage());
	ctx.impact->Shake(kLandShake);

	m_LeapsLeft--;
	if (m_LeapsLeft > 0)
	{
		// Straight on into the next leap, aimed from where it came down.
		float playerCol, playerRow;
		WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
		Start(Blow::Leap, Tuning::GodPiece::kChainWindup / FurySpeed());
		m_FromCol = col;
		m_FromRow = row;
		ChooseLeap(playerCol - col, playerRow - row);
		return;
	}

	Finish();
}

void GodPieceActor::StrikeBoard(const ActorContext& ctx)
{
	int playerCol, playerRow;
	WorldGrid::WorldToTile(ctx.playerX, ctx.playerY, playerCol, playerRow);
	if (IsMarkedSquare(playerCol, playerRow))
	{
		Combat::StrikePlayerRadius(ctx, ctx.playerX, ctx.playerY, 1.f, Tuning::GodPiece::kBoardDamage * FuryDamage());
	}
	ctx.impact->Shake(kLandShake);
}

bool GodPieceActor::IsMarkedSquare(int col, int row) const
{
	int dc = col - m_BoardCol;
	int dr = row - m_BoardRow;
	int radius = Tuning::GodPiece::kBoardRadius;
	if (dc * dc + dr * dr > radius * radius || (dc == 0 && dr == 0))
	{
		return false;
	}
	return ((col + row) & 1) == m_BoardParity;
}

void GodPieceActor::Finish()
{
	m_MoveCount++;
	m_Cooldown = Tuning::GodPiece::kCooldown / FurySpeed();
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void GodPieceActor::OnReset(const ActorContext& ctx)
{
	m_State = State::Wait;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
	m_MoveCount = 0;
	m_Lift = 0.f;
}

void GodPieceActor::Reward(const ActorContext& ctx)
{
	ScatterMotes(ctx, Alignment::Light, Tuning::GodPiece::kRewardMotes);
	ScatterMotes(ctx, Alignment::Dark, Tuning::GodPiece::kRewardMotes);
}

void GodPieceActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
	float progress = m_StateTime / m_Windup;
	if (progress > 1.f)
	{
		progress = 1.f;
	}
	const Palette::Color& mark = MarkColor();

	if (m_Blow == Blow::Board)
	{
		int radius = Tuning::GodPiece::kBoardRadius;
		float alpha = kTelegraphBase + kTelegraphFill * progress;
		for (int row = m_BoardRow - radius; row <= m_BoardRow + radius; row++)
		{
			for (int col = m_BoardCol - radius; col <= m_BoardCol + radius; col++)
			{
				if (!IsMarkedSquare(col, row))
				{
					continue;
				}
				float worldX, worldY, squareX, squareY;
				WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);
				ctx.ToScreen(worldX, worldY, squareX, squareY);
				out.AddDiamond((float)(col + row) + 0.03f, squareX, squareY + WorldGrid::GroundHeightAt(worldX, worldY),
					WorldGrid::kTileWidth - 6.f, WorldGrid::kTileHeight - 3.f, mark.r, mark.g, mark.b, alpha);
			}
		}
		return;
	}

	float lineCol = m_ToCol - m_FromCol;
	float lineRow = m_ToRow - m_FromRow;
	float length = sqrtf(lineCol * lineCol + lineRow * lineRow);
	float baseY = groundY + kTelegraphLift;

	if (m_Blow == Blow::Slide)
	{
		if (length < 0.001f)
		{
			return;
		}
		lineCol /= length;
		lineRow /= length;
		out.AddGroundStrip(WorldX(), WorldY(), x, baseY, lineCol, lineRow, 0.f, length, Tuning::GodPiece::kSlideWidth,
			mark.r, mark.g, mark.b, kTelegraphBase);
		out.AddGroundStrip(WorldX(), WorldY(), x, baseY, lineCol, lineRow, 0.f, length * progress, Tuning::GodPiece::kSlideWidth,
			mark.r, mark.g, mark.b, kTelegraphFill);
		return;
	}

	// The square it will come down on, placed from its own feet.
	float offsetX, offsetY;
	WorldGrid::IsoPosition(lineCol, lineRow, offsetX, offsetY);
	float landX = WorldX() + offsetX;
	float landY = WorldY() + offsetY;
	float radius = Tuning::GodPiece::kLandRadius;
	out.AddGroundArc(landX, landY, x + offsetX, baseY + offsetY, 0.f, radius, 0.f, 2.f * kPi, kTelegraphSegments,
		mark.r, mark.g, mark.b, kTelegraphBase);
	out.AddGroundArc(landX, landY, x + offsetX, baseY + offsetY, 0.f, radius * progress, 0.f, 2.f * kPi, kTelegraphSegments,
		mark.r, mark.g, mark.b, kTelegraphFill);
	out.AddGroundArc(landX, landY, x + offsetX, baseY + offsetY, radius - kTelegraphEdgeWidth, radius, 0.f, 2.f * kPi,
		kTelegraphSegments, mark.r, mark.g, mark.b, kTelegraphEdge);
}

void GodPieceActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 260.f, 260.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	if (m_State == State::Windup)
	{
		DrawTelegraph(out, ctx, x, groundY);
	}

	float shadowScale = 1.f - m_Lift / 120.f;
	out.AddShadow(depth - 0.01f, x, groundY, 70.f * shadowScale, 35.f * shadowScale);
	DrawFury(out, depth - 0.005f, x, groundY, 70.f);

	// Its ward, flickering to the other colour just before it turns over.
	Alignment ward = m_Ward;
	if (m_WardTimer < Tuning::GodPiece::kWardWarning && fmodf(Time(), kWardFlickerRate * 2.f) < kWardFlickerRate)
	{
		ward = (ward == Alignment::Light) ? Alignment::Dark : Alignment::Light;
	}
	float wardR, wardG, wardB;
	AlignmentColor::Core(ward, wardR, wardG, wardB);
	float wardGlow = (ward == Alignment::Light) ? 0.8f : 0.f;

	out.SetGlow(wardGlow);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + kTelegraphLift, 1.05f, 1.2f, 0.f, 2.f * kPi, kTelegraphSegments,
		wardR, wardG, wardB, 0.75f);
	out.SetGlow(0.f);

	out.SetFigure(true);
	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.75f);
	}

	float f = Facing();
	float base = groundY + m_Lift;

	// A stepped pedestal, a column narrowing as it rises, and a collar.
	out.AddTaperedBox(depth, x, base, 1.4f, 1.4f, 6.f, 0.9f, 0.f, 0.f, kStone.r * 0.9f, kStone.g * 0.9f, kStone.b * 0.9f);
	out.AddTaperedBox(depth + 0.005f, x, base + 6.f, 1.15f, 1.15f, 5.f, 0.85f, 0.f, 0.f, kStone.r, kStone.g, kStone.b);
	out.AddTaperedBox(depth + 0.01f, x, base + 11.f, 0.75f, 0.75f, 30.f, 0.75f, 0.f, 0.f, kStone.r, kStone.g, kStone.b);
	out.AddTaperedBox(depth + 0.015f, x, base + 41.f, 0.95f, 0.95f, 4.f, 1.f, 0.f, 0.f,
		kStone.r * 1.05f, kStone.g * 1.05f, kStone.b * 1.05f);

	// Seams down the column, lit in the colour of its ward.
	out.SetGlow(wardGlow * 0.9f);
	out.AddRect(depth + 0.02f, x - 4.f, base + 26.f, 1.6f, 22.f, wardR, wardG, wardB, 1.f);
	out.AddRect(depth + 0.02f, x + 5.f, base + 24.f, 1.6f, 18.f, wardR, wardG, wardB, 1.f);
	out.SetGlow(0.f);

	if (m_Kind == Kind::Rook)
	{
		// A battlemented crown.
		out.AddTaperedBox(depth + 0.02f, x, base + 45.f, 0.9f, 0.9f, 12.f, 1.08f, 0.f, 0.f, kStone.r, kStone.g, kStone.b);
		float reach = 0.97f * WorldGrid::kTileWidth / 2.f;
		float rise = 0.97f * WorldGrid::kTileHeight / 2.f;
		float top = base + 57.f;
		out.AddBox(depth + 0.025f, x, top + rise, 0.22f, 0.22f, 6.f, kStone.r, kStone.g, kStone.b);
		out.AddBox(depth + 0.03f, x - reach, top, 0.22f, 0.22f, 6.f, kStone.r, kStone.g, kStone.b);
		out.AddBox(depth + 0.03f, x + reach, top, 0.22f, 0.22f, 6.f, kStone.r, kStone.g, kStone.b);
		out.AddBox(depth + 0.035f, x, top - rise, 0.22f, 0.22f, 6.f, kStone.r, kStone.g, kStone.b);
		out.AddRect(depth + 0.04f, x + f * 6.f, base + 51.f, 6.f, 1.6f, wardR, wardG, wardB, 1.f);
	}
	else
	{
		// A horse's head thrust forward on a thick neck, ears laid back.
		out.AddTaperedBox(depth + 0.02f, x, base + 45.f, 0.6f, 0.5f, 14.f, 0.8f, f * 5.f, 0.f, kStone.r, kStone.g, kStone.b);
		out.AddTaperedBox(depth + 0.03f, x + f * 9.f, base + 55.f, 0.45f, 0.35f, 9.f, 0.7f, f * 7.f, 0.f,
			kStone.r * 1.05f, kStone.g * 1.05f, kStone.b * 1.05f);
		out.AddTaperedBox(depth + 0.035f, x + f * 1.f, base + 59.f, 0.06f, 0.06f, 7.f, 0.2f, -f * 2.f, 0.f,
			kStone.r, kStone.g, kStone.b);
		out.AddTaperedBox(depth + 0.035f, x + f * 5.f, base + 59.f, 0.06f, 0.06f, 7.f, 0.2f, -f * 2.f, 0.f,
			kStone.r, kStone.g, kStone.b);
		out.AddRect(depth + 0.04f, x + f * 12.f, base + 60.f, 1.8f, 1.6f, wardR, wardG, wardB, 1.f);
	}
}
