#include "stdafx.h"
#include "GodActor.h"
#include "EnemyProjectileActor.h"
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

	const float kStrikeShowTime = 0.3f;
	const float kFallTime = 1.2f;

	// The empty place hangs this high over the ground it guards.
	const float kVoidHeight = 190.f;
	const float kVoidRadius = 95.f;    // pixels, the dark of it
	const float kVoidRim = 2.3f;       // tiles, the ring round it

	// The hands: how far round the empty place they circle, how fast, how
	// high they float, and how quickly they move to where they are going.
	const float kHandOrbit = 4.f;
	const float kHandOrbitRate = 0.5f;
	const float kHandRestLift = 70.f;
	const float kHandRaisedLift = 120.f;
	const float kHandSweepLift = 16.f;
	const float kHandChase = 6.f;      // share of the way per second
	const float kSweepTravel = 9.f;    // tiles a sweeping hand carries on after striking

	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;

	const int kShiftSparks = 24;
	const float kShiftSparkSpeed = 240.f;
	const float kSparkLift = 60.f;

	SparkActor::Style SparkStyleFor(Alignment alignment)
	{
		switch (alignment)
		{
		case Alignment::Light:
		{
			return SparkActor::Style::Light;
		}
		case Alignment::Dark:
		{
			return SparkActor::Style::Dark;
		}
		default:
		{
			return SparkActor::Style::Dust;
		}
		}
	}
}

GodActor::GodActor(int region)
	: BossActor("「   」", Tuning::God::kMaxHealth, region, Tuning::God::kWakeRange, Tuning::God::kLeashRange,
		Tuning::God::kFuryTime)
{
}

float GodActor::BodyRadius() const
{
	return Tuning::God::kBodyRadius;
}

float GodActor::TakeHit(float damage)
{
	if (!m_Provoked)
	{
		return 0.f;
	}
	return BossActor::TakeHit(damage);
}

const char* GodActor::Hint() const
{
	switch (m_Phase)
	{
	case 0:
	{
		return "빛만이 닿는다";
	}
	case 1:
	{
		return "어둠만이 닿는다";
	}
	default:
	{
		return "잿빛만이 닿는다";
	}
	}
}

void GodActor::Provoke()
{
	m_Provoked = true;
	RequestWake();
}

bool GodActor::HasFallen() const
{
	return m_Fallen;
}

int GodActor::PhaseFor(float healthFraction) const
{
	if (healthFraction > 2.f / 3.f)
	{
		return 0;
	}
	if (healthFraction > 1.f / 3.f)
	{
		return 1;
	}
	return 2;
}

Form GodActor::WardForm() const
{
	return (m_Phase == 0) ? Form::Light : ((m_Phase == 1) ? Form::Dark : Form::Ash);
}

Alignment GodActor::WardAlignment() const
{
	return FormAlignment(WardForm());
}

float GodActor::Resist(float damage)
{
	// Only the power its ward is open to reaches it, and never while the ward
	// is turning over.
	if (m_State == State::Shift || PlayerForm() != WardForm())
	{
		return 0.f;
	}

	// And no blow carries it past the next ward: that one has to be opened
	// with its own power.
	if (m_Phase < 2)
	{
		float health = HealthFraction() * Tuning::God::kMaxHealth;
		float wardEdge = Tuning::God::kMaxHealth * (2 - m_Phase) / 3.f;
		if (damage > health - wardEdge)
		{
			damage = health - wardEdge;
		}
	}
	return damage;
}

void GodActor::OnUpdate(const ActorContext& ctx)
{
	m_Clock += ctx.deltaTime;
	UpdateHands(ctx.deltaTime);

	if (m_Fallen)
	{
		m_FallenTime += ctx.deltaTime;
		return;
	}
	if (!m_Provoked)
	{
		return;
	}
	BossActor::OnUpdate(ctx);
}

void GodActor::Defeated(const ActorContext& ctx)
{
	m_Fallen = true;
	m_FallenTime = 0.f;
	ctx.impact->HitStop(Tuning::Boss::kDefeatStop);
	ctx.impact->Shake(Tuning::Boss::kDefeatShake);
}

void GodActor::Reward(const ActorContext& ctx)
{
}

void GodActor::OnReset(const ActorContext& ctx)
{
	m_State = State::Hover;
	m_StateTime = 0.f;
	m_Cooldown = 1.5f;
	m_BlowCount = 0;
	m_StrikeFlash = 0.f;
	m_Phase = PhaseFor(HealthFraction());
}

void GodActor::RestingPlace(int hand, float& col, float& row) const
{
	float coreCol, coreRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), coreCol, coreRow);
	float angle = m_Clock * kHandOrbitRate + hand * kPi;
	col = coreCol + cosf(angle) * kHandOrbit;
	row = coreRow + sinf(angle) * kHandOrbit;
}

void GodActor::UpdateHands(float deltaTime)
{
	for (int hand = 0; hand < kHands; hand++)
	{
		float goalCol, goalRow;
		RestingPlace(hand, goalCol, goalRow);
		float goalLift = kHandRestLift;

		bool active = (hand == m_ActiveHand) && !m_Fallen && m_Provoked;
		if (active && (m_Blow == Blow::Place || m_Blow == Blow::Sweep))
		{
			if (m_State == State::Windup)
			{
				goalCol = m_TargetCol;
				goalRow = m_TargetRow;
				goalLift = (m_Blow == Blow::Place) ? kHandRaisedLift : kHandSweepLift;
			}
			else if (m_State == State::Recover)
			{
				// Down on the board, or carried on along the sweep.
				goalCol = m_TargetCol;
				goalRow = m_TargetRow;
				goalLift = 0.f;
				if (m_Blow == Blow::Sweep)
				{
					goalCol += m_SweepCol * kSweepTravel;
					goalRow += m_SweepRow * kSweepTravel;
					goalLift = kHandSweepLift;
				}
			}
		}

		// Fallen, the hands drop to the board and lie there.
		if (m_Fallen)
		{
			goalCol = m_HandCol[hand];
			goalRow = m_HandRow[hand];
			goalLift = 0.f;
		}

		if (!m_HandsPlaced)
		{
			m_HandCol[hand] = goalCol;
			m_HandRow[hand] = goalRow;
			m_HandLift[hand] = goalLift;
			continue;
		}

		float follow = kHandChase * deltaTime;
		if (follow > 1.f)
		{
			follow = 1.f;
		}
		m_HandCol[hand] += (goalCol - m_HandCol[hand]) * follow;
		m_HandRow[hand] += (goalRow - m_HandRow[hand]) * follow;
		m_HandLift[hand] += (goalLift - m_HandLift[hand]) * follow;
	}
	m_HandsPlaced = true;
}

void GodActor::Fight(const ActorContext& ctx)
{
	m_StateTime += ctx.deltaTime;
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_StrikeFlash > 0.f)
	{
		m_StrikeFlash -= ctx.deltaTime / kStrikeShowTime;
	}

	// A ward broken: it turns over to the next power before anything else.
	int phase = PhaseFor(HealthFraction());
	if (phase != m_Phase)
	{
		m_Phase = phase;
		m_State = State::Shift;
		m_StateTime = 0.f;
		ctx.impact->Shake(Tuning::God::kShiftShake);
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkStyleFor(WardAlignment()), kShiftSparks, kShiftSparkSpeed,
			kSparkLift);
		return;
	}

	switch (m_State)
	{
	case State::Hover:
	{
		if (m_Cooldown <= 0.f)
		{
			Choose(ctx);
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
		if (m_StateTime >= Tuning::God::kRecover / FurySpeed())
		{
			m_State = State::Hover;
			m_StateTime = 0.f;
		}
		break;
	}
	case State::Shift:
	{
		if (m_StateTime >= Tuning::God::kShiftTime)
		{
			m_State = State::Hover;
			m_StateTime = 0.f;
			m_Cooldown = Tuning::God::kCooldown;
		}
		break;
	}
	}
}

void GodActor::Choose(const ActorContext& ctx)
{
	float playerCol, playerRow;
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	m_TargetCol = playerCol;
	m_TargetRow = playerRow;
	m_ActiveHand = m_BlowCount % kHands;

	// What it reaches for grows with each ward: the hands alone, then the
	// board, then the empty place itself.
	int step = m_BlowCount % 4;
	if (step == 3 && m_Phase >= 1)
	{
		m_BoardCol = (int)floorf(playerCol + 0.5f);
		m_BoardRow = (int)floorf(playerRow + 0.5f);
		m_BoardParity = m_BlowCount % 2;
		Start(Blow::Board, Tuning::God::kBoardWindup);
		return;
	}
	if (step == 2 && m_Phase >= 2)
	{
		Start(Blow::Ring, Tuning::God::kRingWindup);
		return;
	}
	if (step % 2 == 1)
	{
		// Across the line from the empty place to him, so stepping back from
		// it does not help.
		float coreCol, coreRow;
		WorldGrid::InverseIso(WorldX(), WorldY(), coreCol, coreRow);
		float toCol = playerCol - coreCol;
		float toRow = playerRow - coreRow;
		float length = sqrtf(toCol * toCol + toRow * toRow);
		if (length < 0.001f)
		{
			toCol = 1.f;
			length = 1.f;
		}
		m_SweepCol = -toRow / length;
		m_SweepRow = toCol / length;
		Start(Blow::Sweep, Tuning::God::kSweepWindup);
		return;
	}
	Start(Blow::Place, Tuning::God::kPlaceWindup);
}

void GodActor::Start(Blow blow, float windup)
{
	m_Blow = blow;
	if (m_Phase == 2)
	{
		windup *= Tuning::God::kLastWardWindupScale;
	}
	m_Windup = windup / FurySpeed();
	m_State = State::Windup;
	m_StateTime = 0.f;
}

void GodActor::Release(const ActorContext& ctx)
{
	switch (m_Blow)
	{
	case Blow::Place:
	{
		float x, y;
		WorldGrid::IsoPosition(m_TargetCol, m_TargetRow, x, y);
		Combat::StrikePlayerRadius(ctx, x, y, Tuning::God::kPlaceRadius, Tuning::God::kPlaceDamage * FuryDamage());
		ctx.impact->Shake(Tuning::God::kPlaceShake);
		break;
	}
	case Blow::Sweep:
	{
		float half = Tuning::God::kSweepLength * 0.5f;
		float x, y;
		WorldGrid::IsoPosition(m_TargetCol - m_SweepCol * half, m_TargetRow - m_SweepRow * half, x, y);
		Combat::StrikePlayerLine(ctx, x, y, m_SweepCol, m_SweepRow, Tuning::God::kSweepLength,
			Tuning::God::kSweepWidth, Tuning::God::kSweepDamage * FuryDamage());
		break;
	}
	case Blow::Board:
	{
		// Every square of one colour round where he stood strikes at once.
		float playerCol, playerRow;
		WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
		int col = (int)floorf(playerCol + 0.5f);
		int row = (int)floorf(playerRow + 0.5f);
		int radius = Tuning::God::kBoardRadius;
		bool inside = abs(col - m_BoardCol) <= radius && abs(row - m_BoardRow) <= radius;
		if (inside && ((col + row) % 2 + 2) % 2 == m_BoardParity)
		{
			Combat::StrikePlayerRadius(ctx, ctx.playerX, ctx.playerY, 1.f, Tuning::God::kBoardDamage * FuryDamage());
		}
		ctx.impact->Shake(Tuning::God::kPlaceShake);
		break;
	}
	case Blow::Ring:
	{
		// Light, dark and the gray by turns: everything on the board is its.
		int count = Tuning::God::kRingCount;
		float offset = (m_BlowCount % 8 < 4) ? 0.f : 0.5f;
		for (int i = 0; i < count; i++)
		{
			float angle = (i + offset) * 2.f * kPi / count;
			Alignment alignment = (i % 3 == 0) ? Alignment::Light : ((i % 3 == 1) ? Alignment::Dark : Alignment::Ashen);
			std::unique_ptr<EnemyProjectileActor> shot(new EnemyProjectileActor(alignment, cosf(angle), sinf(angle),
				Tuning::God::kRingSpeed * FurySpeed(), 0.f, Tuning::God::kRingLifetime,
				Tuning::God::kRingDamage * FuryDamage()));
			shot->SetLocalPosition(WorldX(), WorldY());
			ctx.graph->Spawn(std::move(shot));
		}
		break;
	}
	}

	m_StrikeFlash = 1.f;
	float cooldown = Tuning::God::kCooldown * ((m_Phase == 2) ? Tuning::God::kLastWardWindupScale : 1.f);
	m_Cooldown = cooldown / FurySpeed();
	m_BlowCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void GodActor::DrawTelegraph(DrawList& out, const ActorContext& ctx) const
{
	float progress = m_StateTime / m_Windup;
	if (progress > 1.f)
	{
		progress = 1.f;
	}
	float r, g, b;
	AlignmentColor::Warning(WardAlignment(), Palette::kWarningWash, r, g, b);

	float targetX, targetY, screenX, screenY;
	WorldGrid::IsoPosition(m_TargetCol, m_TargetRow, targetX, targetY);
	ctx.ToScreen(targetX, targetY, screenX, screenY);
	screenY += WorldGrid::GroundHeightAt(targetX, targetY) + kTelegraphLift;

	switch (m_Blow)
	{
	case Blow::Place:
	{
		float radius = Tuning::God::kPlaceRadius;
		out.AddGroundArc(targetX, targetY, screenX, screenY, 0.f, radius, 0.f, 2.f * kPi, kTelegraphSegments,
			r, g, b, kTelegraphBase);
		out.AddGroundArc(targetX, targetY, screenX, screenY, 0.f, radius * progress, 0.f, 2.f * kPi, kTelegraphSegments,
			r, g, b, kTelegraphFill);
		out.AddGroundArc(targetX, targetY, screenX, screenY, radius - kTelegraphEdgeWidth, radius, 0.f, 2.f * kPi,
			kTelegraphSegments, r, g, b, kTelegraphEdge);
		break;
	}
	case Blow::Sweep:
	{
		float length = Tuning::God::kSweepLength;
		float half = length * 0.5f;
		float startX, startY, startScreenX, startScreenY;
		WorldGrid::IsoPosition(m_TargetCol - m_SweepCol * half, m_TargetRow - m_SweepRow * half, startX, startY);
		ctx.ToScreen(startX, startY, startScreenX, startScreenY);
		startScreenY += WorldGrid::GroundHeightAt(startX, startY) + kTelegraphLift;
		out.AddGroundStrip(startX, startY, startScreenX, startScreenY, m_SweepCol, m_SweepRow, 0.f, length,
			Tuning::God::kSweepWidth, r, g, b, kTelegraphBase);
		out.AddGroundStrip(startX, startY, startScreenX, startScreenY, m_SweepCol, m_SweepRow, 0.f, length * progress,
			Tuning::God::kSweepWidth, r, g, b, kTelegraphFill);
		break;
	}
	case Blow::Board:
	{
		int radius = Tuning::God::kBoardRadius;
		float alpha = kTelegraphBase + kTelegraphFill * progress;
		for (int row = m_BoardRow - radius; row <= m_BoardRow + radius; row++)
		{
			for (int col = m_BoardCol - radius; col <= m_BoardCol + radius; col++)
			{
				if (((col + row) % 2 + 2) % 2 != m_BoardParity)
				{
					continue;
				}
				float worldX, worldY, squareX, squareY;
				WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);
				ctx.ToScreen(worldX, worldY, squareX, squareY);
				out.AddDiamond((float)(col + row) + 0.03f, squareX, squareY + WorldGrid::GroundHeightAt(worldX, worldY),
					WorldGrid::kTileWidth - 6.f, WorldGrid::kTileHeight - 3.f, r, g, b, alpha);
			}
		}
		break;
	}
	default:
	{
		float x, y;
		ctx.ToScreen(WorldX(), WorldY(), x, y);
		float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + kTelegraphLift;
		float radius = Tuning::God::kBodyRadius + progress * 2.f;
		out.AddGroundArc(WorldX(), WorldY(), x, groundY, radius - 0.15f, radius, 0.f, 2.f * kPi, kTelegraphSegments,
			r, g, b, 0.45f * progress);
		break;
	}
	}
}

void GodActor::DrawVoid(DrawList& out, float depth, float x, float groundY) const
{
	float fall = m_Fallen ? ((m_FallenTime < kFallTime) ? m_FallenTime / kFallTime : 1.f) : 0.f;
	float pulse = 0.85f + 0.15f * sinf(m_Clock * 1.3f);
	bool shifting = (m_State == State::Shift) && !m_Fallen;
	float flicker = shifting ? 0.5f + 0.5f * sinf(m_Clock * 30.f) : 1.f;
	float voidY = groundY + kVoidHeight;

	// The empty place: a hole of nothing in the sky, darkening what is
	// behind it, closing as it falls.
	float size = 1.f - fall;
	out.AddGlow(depth + 2.f, x, voidY, kVoidRadius * size * pulse, Palette::kVoid.r, Palette::kVoid.g, Palette::kVoid.b, 1.f);
	out.AddGlow(depth + 2.01f, x, voidY, kVoidRadius * 0.55f * size, 0.f, 0.f, 0.f, 1.f);

	// Its rim, and the ring on the ground under it where it can be struck,
	// both in the colour of the one power its ward lets through.
	float r, g, b;
	AlignmentColor::Core(WardAlignment(), r, g, b);
	float rimAlpha = 0.8f * flicker * size;
	out.SetGlow(WardAlignment() == Alignment::Dark ? 0.f : 0.8f);
	out.AddGroundArc(WorldX(), WorldY(), x, voidY, kVoidRim * size, (kVoidRim + 0.15f) * size, 0.f, 2.f * kPi,
		kTelegraphSegments, r, g, b, rimAlpha);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + kTelegraphLift, Tuning::God::kBodyRadius - 0.1f,
		Tuning::God::kBodyRadius, 0.f, 2.f * kPi, kTelegraphSegments, r, g, b, 0.6f * flicker * size);
	out.SetGlow(0.f);
}

void GodActor::DrawHand(DrawList& out, const ActorContext& ctx, int hand) const
{
	float worldX, worldY, x, y;
	WorldGrid::IsoPosition(m_HandCol[hand], m_HandRow[hand], worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	if (!ctx.IsOnScreen(x, y, 80.f, 200.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(worldX, worldY);
	float depth = WorldGrid::TileDepth(worldX, worldY) + 0.6f;
	float lift = m_HandLift[hand];
	const Palette::Color& skin = Palette::kGodHand;

	// Its shadow grows sharper as it comes down.
	float closeness = 1.f - lift / kHandRaisedLift;
	out.AddShadow(depth - 0.01f, x, groundY, 40.f + 20.f * closeness, 20.f + 10.f * closeness);

	// A palm turned down, four long fingers and a thumb hanging from it.
	out.SetFigure(true);
	float palmY = groundY + lift + 14.f;
	float side = (hand == 0) ? -1.f : 1.f;
	out.AddTaperedBox(depth, x, palmY, 0.5f, 0.32f, 14.f, 0.85f, 0.f, 0.f, skin.r, skin.g, skin.b);
	for (int i = 0; i < 4; i++)
	{
		float fingerX = x + (i - 1.5f) * 6.f;
		out.AddTaperedBox(depth + 0.005f, fingerX, palmY - 13.f, 0.07f, 0.07f, 14.f, 0.6f, 0.f, 0.f,
			skin.r * 0.95f, skin.g * 0.95f, skin.b * 0.95f);
	}
	out.AddTaperedBox(depth + 0.005f, x + side * 15.f, palmY - 4.f, 0.07f, 0.07f, 10.f, 0.6f, side * 3.f, 0.f,
		skin.r * 0.95f, skin.g * 0.95f, skin.b * 0.95f);
	out.SetFigure(false);
}

void GodActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	if (m_Provoked && !m_Fallen && IsAwake() && m_State == State::Windup)
	{
		DrawTelegraph(out, ctx);
	}

	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.5f);
	}
	DrawFury(out, depth - 0.005f, x, groundY, 110.f);
	DrawVoid(out, depth, x, groundY);
	out.SetFlash(0.f);

	for (int hand = 0; hand < kHands; hand++)
	{
		DrawHand(out, ctx, hand);
	}
}
