#include "stdafx.h"
#include "GodActor.h"
#include "GodMarkActor.h"
#include "WardCoreActor.h"
#include "EnemyProjectileActor.h"
#include "CurseFieldActor.h"
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
	const float kPi = 3.14159265f;
	const float kDegToRad = kPi / 180.f;

	const float kStrikeShowTime = 0.3f;
	const float kFallTime = 1.2f;

	// The giant: a body rising from beyond the board's far corner, sunk a
	// little below it, its top dropping forward as it bows; shoulders, a
	// head, and the mark turning where its face should be.
	const float kTorsoSink = 30.f;
	const float kTorsoHeight = 210.f;
	const float kTorsoCols = 3.2f;
	const float kTorsoRows = 2.f;
	const float kTorsoFlare = 1.3f;
	const float kBowDrop = 70.f;        // pixels its shoulders come down, bowed
	const float kShoulderSpan = 62.f;   // pixels either side, where the arms hang from
	const float kHeadHeight = 64.f;
	const float kHeadFoot = 1.3f;
	const float kMarkSize = 30.f;
	const float kMarkSpin = 0.6f;       // radians a second
	const float kArmShoulder = 22.f;    // pixels wide at the shoulder
	const float kArmWrist = 12.f;
	const float kBodyShine = 0.08f;
	const float kWardAura = 140.f;

	// The hands: either side of the far corner, swaying, and how high they
	// float, and how quickly they move to where they are going.
	const float kHandSway = 0.6f;
	const float kHandSwayRate = 0.6f;
	const float kHandRestLift = 70.f;
	const float kHandRaisedLift = 120.f;
	const float kHandSweepLift = 16.f;
	const float kHandChase = 6.f;      // share of the way per second
	const float kSweepTravel = 9.f;    // tiles a sweeping hand carries on after striking
	const float kHandRest[2][2] = { { -4.5f, 0.5f }, { 0.5f, -4.5f } }; // tiles from the centre

	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;
	const float kVolleyLineLength = 10.f;
	const float kVolleyLineWidth = 0.3f;

	const int kShiftSparks = 24;
	const float kShiftSparkSpeed = 240.f;
	const float kSparkLift = 60.f;

	// What it throws at him, ward by ward - growing with each.
	const int kWardBlows0[] = { 0, 1, 2 };          // Place, Sweep, Volley
	const int kWardBlows1[] = { 0, 3, 1, 4, 2 };    // Place, Board, Sweep, Mire, Volley
	const int kWardBlows2[] = { 5, 0, 3, 1, 2, 4 }; // Ring, Place, Board, Sweep, Volley, Mire

	SparkActor::Style SparkStyleFor(Alignment alignment)
	{
		return (alignment == Alignment::Dark) ? SparkActor::Style::Dark : SparkActor::Style::Light;
	}

	float Smooth(float t)
	{
		return t * t * (3.f - 2.f * t);
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
	if (m_State == State::Shift)
	{
		return "결계가 바뀐다";
	}
	if (IsGapOpen())
	{
		return "틈이 열렸다 · 숙인 얼굴을 베어라";
	}
	switch (m_Phase)
	{
	case 0:
	{
		return "빛의 결계 · 핵을 풀어내라 (E)";
	}
	case 1:
	{
		return "어둠의 결계 · 핵을 풀어내라 (E)";
	}
	default:
	{
		return "흰 결계 · 핵을 풀어내라 (E)";
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

Alignment GodActor::WardAlignment() const
{
	return (m_Phase == 0) ? Alignment::Light : ((m_Phase == 1) ? Alignment::Dark : Alignment::Divine);
}

bool GodActor::IsGapOpen() const
{
	return m_GapTimer > 0.f;
}

void GodActor::CenterTile(float& col, float& row) const
{
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	col -= Tuning::God::kStandOffset;
	row -= Tuning::God::kStandOffset;
}

void GodActor::FacePoint(float& x, float& y) const
{
	float col, row;
	CenterTile(col, row);
	WorldGrid::IsoPosition(col + Tuning::God::kFaceOffset, row + Tuning::God::kFaceOffset, x, y);
}

float GodActor::Resist(float damage)
{
	// Nothing reaches it while its ward stands, nor while it turns over.
	if (!IsGapOpen() || m_State == State::Shift)
	{
		return 0.f;
	}

	// And no blow carries it past the next ward.
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

void GodActor::RaiseWard(const ActorContext& ctx)
{
	ClearCores();
	float centerCol, centerRow;
	CenterTile(centerCol, centerRow);

	int first = 0;
	for (int i = 0; i < m_Phase; i++)
	{
		first += Tuning::God::kCoresPerWard[i];
	}
	m_CoreCount = Tuning::God::kCoresPerWard[m_Phase];
	for (int i = 0; i < m_CoreCount && i < kMaxCores; i++)
	{
		const float* spot = Tuning::God::kCoreSpots[first + i];
		float x, y;
		WorldGrid::IsoPosition(centerCol + spot[0], centerRow + spot[1], x, y);
		std::unique_ptr<WardCoreActor> core(new WardCoreActor(WardAlignment()));
		core->SetLocalPosition(x, y);
		m_Cores[i] = core.get();
		ctx.graph->Spawn(std::move(core));
	}
	m_WardUp = true;
}

void GodActor::ClearCores()
{
	for (int i = 0; i < kMaxCores; i++)
	{
		if (m_Cores[i] != nullptr)
		{
			m_Cores[i]->Destroy();
			m_Cores[i] = nullptr;
		}
	}
	m_CoreCount = 0;
	m_WardUp = false;
}

int GodActor::CoresLeft() const
{
	int left = 0;
	for (int i = 0; i < m_CoreCount; i++)
	{
		if (m_Cores[i] != nullptr && !m_Cores[i]->IsBroken())
		{
			left++;
		}
	}
	return left;
}

void GodActor::OpenGap(const ActorContext& ctx)
{
	ClearCores();
	m_GapTimer = Tuning::God::kGapTime;
	m_State = State::Hover;
	m_StateTime = 0.f;
	ctx.impact->Shake(Tuning::God::kGapShake);
	float x, y;
	FacePoint(x, y);
	SparkActor::Burst(*ctx.graph, x, y, SparkStyleFor(WardAlignment()), kShiftSparks, kShiftSparkSpeed, kSparkLift);
}

void GodActor::OnUpdate(const ActorContext& ctx)
{
	m_Clock += ctx.deltaTime;
	UpdateHands(ctx.deltaTime);

	// Bowed while its ward is broken, and slumped over the board once beaten.
	float bowGoal = (IsGapOpen() || m_Fallen) ? 1.f : 0.f;
	float step = Tuning::God::kBowRate * ctx.deltaTime;
	m_Bow = (m_Bow < bowGoal) ? ((m_Bow + step > bowGoal) ? bowGoal : m_Bow + step)
		: ((m_Bow - step < bowGoal) ? bowGoal : m_Bow - step);

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
	ClearCores();
	m_GapTimer = 0.f;
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
	ClearCores();
	m_GapTimer = 0.f;
	m_State = State::Hover;
	m_StateTime = 0.f;
	m_Cooldown = 1.5f;
	m_BlowCount = 0;
	m_StrikeFlash = 0.f;
	m_Phase = PhaseFor(HealthFraction());
}

void GodActor::RestingPlace(int hand, float& col, float& row) const
{
	float centerCol, centerRow;
	CenterTile(centerCol, centerRow);
	float sway = sinf(m_Clock * kHandSwayRate + hand * kPi) * kHandSway;
	col = centerCol + kHandRest[hand][0] + sway;
	row = centerRow + kHandRest[hand][1] + sway;
}

void GodActor::UpdateHands(float deltaTime)
{
	for (int hand = 0; hand < kHands; hand++)
	{
		float goalCol, goalRow;
		RestingPlace(hand, goalCol, goalRow);
		// Bowed, its palms rest flat on the board.
		float goalLift = IsGapOpen() ? 0.f : kHandRestLift;

		bool active = (hand == m_ActiveHand) && !m_Fallen && m_Provoked && !IsGapOpen();
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

	// That third of its life spent: the ward turns over to the next power
	// before anything else.
	int phase = PhaseFor(HealthFraction());
	if (phase != m_Phase)
	{
		m_Phase = phase;
		m_GapTimer = 0.f;
		ClearCores();
		m_State = State::Shift;
		m_StateTime = 0.f;
		ctx.impact->Shake(Tuning::God::kShiftShake);
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkStyleFor(WardAlignment()), kShiftSparks, kShiftSparkSpeed,
			kSparkLift);
		return;
	}

	if (m_State == State::Shift)
	{
		if (m_StateTime >= Tuning::God::kShiftTime)
		{
			m_State = State::Hover;
			m_StateTime = 0.f;
			m_Cooldown = Tuning::God::kCooldown;
			RaiseWard(ctx);
		}
		return;
	}

	// Bowed to the board, it does nothing but wait for the gap to close.
	if (IsGapOpen())
	{
		m_GapTimer -= ctx.deltaTime;
		if (!IsGapOpen())
		{
			RaiseWard(ctx);
			m_Cooldown = Tuning::God::kCooldown;
		}
		return;
	}
	if (!m_WardUp)
	{
		RaiseWard(ctx);
	}
	else if (CoresLeft() == 0)
	{
		OpenGap(ctx);
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
	default:
	{
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

	const int* blows = kWardBlows0;
	int count = sizeof(kWardBlows0) / sizeof(kWardBlows0[0]);
	if (m_Phase == 1)
	{
		blows = kWardBlows1;
		count = sizeof(kWardBlows1) / sizeof(kWardBlows1[0]);
	}
	else if (m_Phase == 2)
	{
		blows = kWardBlows2;
		count = sizeof(kWardBlows2) / sizeof(kWardBlows2[0]);
	}
	Blow blow = (Blow)blows[m_BlowCount % count];

	switch (blow)
	{
	case Blow::Sweep:
	{
		// Across the line from its face to him, so stepping back from it
		// does not help.
		float faceX, faceY, faceCol, faceRow;
		FacePoint(faceX, faceY);
		WorldGrid::InverseIso(faceX, faceY, faceCol, faceRow);
		float toCol = playerCol - faceCol;
		float toRow = playerRow - faceRow;
		float length = sqrtf(toCol * toCol + toRow * toRow);
		if (length < 0.001f)
		{
			toCol = 1.f;
			length = 1.f;
		}
		m_SweepCol = -toRow / length;
		m_SweepRow = toCol / length;
		Start(Blow::Sweep, Tuning::God::kSweepWindup);
		break;
	}
	case Blow::Volley:
	{
		Start(Blow::Volley, Tuning::God::kVolleyWindup);
		break;
	}
	case Blow::Board:
	{
		m_BoardCol = (int)floorf(playerCol + 0.5f);
		m_BoardRow = (int)floorf(playerRow + 0.5f);
		m_BoardParity = m_BlowCount % 2;
		Start(Blow::Board, Tuning::God::kBoardWindup);
		break;
	}
	case Blow::Mire:
	{
		Start(Blow::Mire, Tuning::God::kMireWindup);
		break;
	}
	case Blow::Ring:
	{
		Start(Blow::Ring, Tuning::God::kRingWindup);
		break;
	}
	default:
	{
		Start(Blow::Place, Tuning::God::kPlaceWindup);
		break;
	}
	}
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
	float faceX, faceY;
	FacePoint(faceX, faceY);

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
	case Blow::Volley:
	{
		// A fan of its ward's power from its face, centred on where he stood.
		float faceCol, faceRow;
		WorldGrid::InverseIso(faceX, faceY, faceCol, faceRow);
		float center = atan2f(m_TargetRow - faceRow, m_TargetCol - faceCol);
		int count = Tuning::God::kVolleyCount;
		float spread = Tuning::God::kVolleySpread * kDegToRad;
		for (int i = 0; i < count; i++)
		{
			float angle = center - spread * 0.5f + spread * i / (count - 1);
			std::unique_ptr<EnemyProjectileActor> shot(new EnemyProjectileActor(WardAlignment(), cosf(angle), sinf(angle),
				Tuning::God::kVolleySpeed * FurySpeed(), 0.f, Tuning::God::kVolleyLifetime,
				Tuning::God::kVolleyDamage * FuryDamage()));
			shot->SetLocalPosition(faceX, faceY);
			ctx.graph->Spawn(std::move(shot));
		}
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
	case Blow::Mire:
	{
		// Curses round where he stood: one under him, the rest about it.
		for (int i = 0; i < Tuning::God::kMireCount; i++)
		{
			float col = m_TargetCol;
			float row = m_TargetRow;
			if (i > 0)
			{
				float angle = Random::Angle();
				col += cosf(angle) * Tuning::God::kMireSpread;
				row += sinf(angle) * Tuning::God::kMireSpread;
			}
			float x, y;
			WorldGrid::IsoPosition(col, row, x, y);
			std::unique_ptr<CurseFieldActor> field(new CurseFieldActor());
			field->SetLocalPosition(x, y);
			ctx.graph->Spawn(std::move(field));
		}
		break;
	}
	case Blow::Ring:
	{
		// Light, dark and its own white by turns - never the gray.
		int count = Tuning::God::kRingCount;
		float offset = (m_BlowCount % 2 == 0) ? 0.f : 0.5f;
		for (int i = 0; i < count; i++)
		{
			float angle = (i + offset) * 2.f * kPi / count;
			Alignment alignment = (i % 3 == 0) ? Alignment::Light : ((i % 3 == 1) ? Alignment::Dark : Alignment::Divine);
			std::unique_ptr<EnemyProjectileActor> shot(new EnemyProjectileActor(alignment, cosf(angle), sinf(angle),
				Tuning::God::kRingSpeed * FurySpeed(), 0.f, Tuning::God::kRingLifetime,
				Tuning::God::kRingDamage * FuryDamage()));
			shot->SetLocalPosition(faceX, faceY);
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

	float faceX, faceY, faceScreenX, faceScreenY;
	FacePoint(faceX, faceY);
	ctx.ToScreen(faceX, faceY, faceScreenX, faceScreenY);
	faceScreenY += WorldGrid::GroundHeightAt(faceX, faceY) + kTelegraphLift;

	switch (m_Blow)
	{
	case Blow::Place:
	case Blow::Mire:
	{
		float radius = (m_Blow == Blow::Place) ? Tuning::God::kPlaceRadius : Tuning::God::kMireSpread;
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
	case Blow::Volley:
	{
		// Thin lines from its face along each shot's way.
		float faceCol, faceRow;
		WorldGrid::InverseIso(faceX, faceY, faceCol, faceRow);
		float center = atan2f(m_TargetRow - faceRow, m_TargetCol - faceCol);
		int count = Tuning::God::kVolleyCount;
		float spread = Tuning::God::kVolleySpread * kDegToRad;
		for (int i = 0; i < count; i++)
		{
			float angle = center - spread * 0.5f + spread * i / (count - 1);
			out.AddGroundStrip(faceX, faceY, faceScreenX, faceScreenY, cosf(angle), sinf(angle), 0.f,
				kVolleyLineLength * progress, kVolleyLineWidth, r, g, b, kTelegraphFill);
		}
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
		// The ring: gathering round its face.
		float radius = 0.5f + progress * 2.f;
		out.AddGroundArc(faceX, faceY, faceScreenX, faceScreenY, radius - 0.15f, radius, 0.f, 2.f * kPi,
			kTelegraphSegments, r, g, b, 0.45f * progress);
		break;
	}
	}
}

void GodActor::HandScreen(const ActorContext& ctx, int hand, float& x, float& y, float& depth) const
{
	float worldX, worldY;
	WorldGrid::IsoPosition(m_HandCol[hand], m_HandRow[hand], worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	y += WorldGrid::GroundHeightAt(worldX, worldY) + m_HandLift[hand];
	depth = WorldGrid::TileDepth(worldX, worldY) + 0.6f;
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
	out.SetGlow(kBodyShine);
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
	out.SetGlow(0.f);
	out.SetFigure(false);
}

void GodActor::DrawBody(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	float faceX, faceY, faceScreenX, faceScreenY;
	FacePoint(faceX, faceY);
	ctx.ToScreen(faceX, faceY, faceScreenX, faceScreenY);
	faceScreenY += WorldGrid::GroundHeightAt(faceX, faceY);
	float faceDepth = WorldGrid::TileDepth(faceX, faceY) + 0.7f;

	float bow = Smooth(m_Bow);
	float fall = m_Fallen ? ((m_FallenTime < kFallTime) ? m_FallenTime / kFallTime : 1.f) : 0.f;
	bool shifting = (m_State == State::Shift) && !m_Fallen;
	float flicker = shifting ? 0.5f + 0.5f * sinf(m_Clock * 30.f) : 1.f;
	const Palette::Color& body = Palette::kGodHand;

	// The power of its ward, hanging about it while the ward stands.
	if (m_WardUp && !m_Fallen)
	{
		float glowR, glowG, glowB;
		AlignmentColor::Glow(WardAlignment(), glowR, glowG, glowB);
		out.AddGlow(depth - 0.01f, x, groundY + kTorsoHeight * 0.6f, kWardAura, glowR, glowG, glowB, 0.6f * flicker);
	}

	// The torso, flaring to the shoulders, its top coming forward and down
	// as it bows.
	out.SetFigure(true);
	out.SetGlow(kBodyShine);
	float base = groundY - kTorsoSink;
	float drop = bow * kBowDrop;
	out.AddTaperedBox(depth, x, base, kTorsoCols, kTorsoRows, kTorsoHeight, kTorsoFlare, 0.f, -drop,
		body.r * 0.92f, body.g * 0.92f, body.b * 0.95f);
	float shoulderY = base + kTorsoHeight - drop;
	out.AddTaperedBox(depth + 0.01f, x, shoulderY - 16.f, kTorsoCols * 1.4f, kTorsoRows * 1.1f, 22.f, 0.7f, 0.f, 0.f,
		body.r, body.g, body.b);

	// The arms, down from its shoulders to its hands on the board - over
	// the squares between, under the hands themselves.
	for (int hand = 0; hand < kHands; hand++)
	{
		float handX, handY, handDepth;
		HandScreen(ctx, hand, handX, handY, handDepth);
		float shoulderX = x + ((hand == 0) ? -kShoulderSpan : kShoulderSpan);
		float wristY = handY + 26.f;
		float dx = handX - shoulderX;
		float dy = wristY - shoulderY;
		float length = sqrtf(dx * dx + dy * dy);
		if (length < 0.001f)
		{
			continue;
		}
		float sideX = -dy / length * 0.5f;
		float sideY = dx / length * 0.5f;
		float arm[8] =
		{
			shoulderX - sideX * kArmShoulder, shoulderY - sideY * kArmShoulder,
			handX - sideX * kArmWrist, wristY - sideY * kArmWrist,
			handX + sideX * kArmWrist, wristY + sideY * kArmWrist,
			shoulderX + sideX * kArmShoulder, shoulderY + sideY * kArmShoulder,
		};
		out.AddCorners(handDepth - 0.02f, arm, body.r * 0.85f, body.g * 0.85f, body.b * 0.88f, 1.f);
	}

	// The head: over its shoulders while it stands, down at the board's
	// corner when it bows.
	float headBase = shoulderY + 6.f + (faceScreenY + 6.f - shoulderY - 6.f) * bow;
	float headX = x + (faceScreenX - x) * bow;
	float headDepth = depth + 0.02f + (faceDepth - depth - 0.02f) * bow;
	out.AddTaperedBox(headDepth, headX, headBase, kHeadFoot, kHeadFoot, kHeadHeight, 0.85f, 0.f, 0.f,
		body.r, body.g, body.b);
	out.SetGlow(0.f);
	out.SetFigure(false);

	// Where its face should be, the altars' mark, turning - guttering as
	// the ward turns over, and going out as it falls.
	GodMarkActor::DrawMark(out, headDepth + 0.01f, headX, headBase + kHeadHeight * 0.55f, kMarkSize,
		m_Clock * kMarkSpin, flicker * (1.f - fall));
}

void GodActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	if (m_Provoked && !m_Fallen && IsAwake() && m_State == State::Windup && !IsGapOpen())
	{
		DrawTelegraph(out, ctx);
	}

	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.5f);
	}
	DrawFury(out, depth - 0.005f, x, groundY, 110.f);
	DrawBody(out, ctx);
	out.SetFlash(0.f);

	for (int hand = 0; hand < kHands; hand++)
	{
		DrawHand(out, ctx, hand);
	}
}
