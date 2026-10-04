#include "stdafx.h"
#include "MonsterActor.h"
#include "MonsterModels.h"
#include "ParticleActor.h"
#include "EnemyProjectileActor.h"
#include "LightningActor.h"
#include "CurseFieldActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "PlayerStatus.h"
#include "Combat.h"
#include "Random.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;

	const float kGaitPerTile = 3.2f; // radians of walk cycle per tile covered
	const float kClimbRate = 14.f;

	// Stops pressing in at this share of its reach while its next blow is
	// not ready, so it circles at arm's length instead of standing on him.
	const float kHoldShare = 0.8f;
	const float kRoamArrive = 0.1f; // tiles

	const float kStrikeShowTime = 0.2f;

	// Struck: how long the white flash takes to fade, how white it starts,
	// and the sparks thrown off.
	const float kHitFlashTime = 0.12f;
	const float kHitFlashStrength = 0.75f;
	const int kHitSparks = 6;
	const float kHitSparkSpeed = 140.f;
	const float kHitSparkLift = 18.f;

	const int kBurstCount = 5;
	const float kBurstSpeed = 90.f;

	// The marked ground: a faint fill of the whole reach, a stronger one
	// growing out to it as the blow gathers, and a bright edge.
	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.06f;
	const int kTelegraphSegments = 10;

	bool IsLight(Alignment alignment)
	{
		return alignment == Alignment::Light;
	}

	bool IsMage(MonsterActor::Kind kind)
	{
		return kind == MonsterActor::Kind::Mage;
	}

	float MaxHealth(MonsterActor::Kind kind, Alignment alignment)
	{
		if (IsMage(kind))
		{
			return IsLight(alignment) ? Tuning::LightMage::kHealth : Tuning::DarkMage::kHealth;
		}
		return IsLight(alignment) ? Tuning::LightKnight::kHealth : Tuning::DarkKnight::kHealth;
	}

	float SpeedScale(MonsterActor::Kind kind, Alignment alignment)
	{
		if (IsMage(kind))
		{
			return IsLight(alignment) ? Tuning::LightMage::kSpeedScale : Tuning::DarkMage::kSpeedScale;
		}
		return IsLight(alignment) ? Tuning::LightKnight::kSpeedScale : Tuning::DarkKnight::kSpeedScale;
	}

	// A knight's reach before it starts a blow.
	float StartRange(Alignment alignment)
	{
		return IsLight(alignment) ? Tuning::LightKnight::kStartRange : Tuning::DarkKnight::kStartRange;
	}

	// The distance a mage likes to keep.
	float PreferredRange(Alignment alignment)
	{
		return IsLight(alignment) ? Tuning::LightMage::kPreferredRange : Tuning::DarkMage::kPreferredRange;
	}

	float RecoverTime(MonsterActor::Kind kind, Alignment alignment)
	{
		if (IsMage(kind))
		{
			return IsLight(alignment) ? Tuning::LightMage::kRecover : Tuning::DarkMage::kRecover;
		}
		return IsLight(alignment) ? Tuning::LightKnight::kRecover : Tuning::DarkKnight::kRecover;
	}

}

MonsterActor::MonsterActor(Kind kind, Alignment alignment)
	: m_Kind(kind)
	, m_Alignment(alignment)
	, m_Health(MaxHealth(kind, alignment))
{
	m_RoamWaitTimer = Random::Unit() * 2.f;
	// Desynchronised, so a group does not sway in lockstep.
	m_Time = Random::Unit() * 10.f;
}

float MonsterActor::TakeHit(float damage)
{
	if (m_Health <= 0.f)
	{
		return 0.f;
	}

	float landed = (damage < m_Health) ? damage : m_Health;
	m_Health -= damage;

	if (landed > 0.f)
	{
		m_HitFlash = 1.f;
		m_SparksPending = true;
	}
	return landed;
}

float MonsterActor::TakeHit(float damage, float fromX, float fromY)
{
	if (m_Kind == Kind::Knight && IsLight(m_Alignment))
	{
		float col, row, fromCol, fromRow;
		WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
		WorldGrid::InverseIso(fromX, fromY, fromCol, fromRow);

		float toCol = fromCol - col;
		float toRow = fromRow - row;
		float length = sqrtf(toCol * toCol + toRow * toRow);
		if (length > 0.001f)
		{
			float dot = (toCol * m_FaceCol + toRow * m_FaceRow) / length;
			if (dot >= cosf(Tuning::Monster::kShieldArcDegrees * 0.5f * kDegToRad))
			{
				damage *= Tuning::Monster::kShieldFactor;
			}
		}
	}

	return TakeHit(damage);
}

void MonsterActor::Shove(float offsetCol, float offsetRow)
{
	float offsetX, offsetY;
	WorldGrid::IsoPosition(offsetCol, offsetRow, offsetX, offsetY);

	m_ShoveVelX = offsetX / Tuning::Shove::kDuration;
	m_ShoveVelY = offsetY / Tuning::Shove::kDuration;
	m_ShoveTimer = Tuning::Shove::kDuration;

	if (m_State == State::Windup || m_State == State::Charge || m_State == State::Recover)
	{
		m_State = State::Chase;
		m_StateTime = 0.f;
		m_SecondCutPending = false;
	}
}

void MonsterActor::OnUpdate(const ActorContext& ctx)
{
	// The spawn position is only known once it has been placed, so the patch it
	// wanders is anchored on the first frame rather than in the constructor.
	if (!m_Anchored)
	{
		WorldGrid::InverseIso(WorldX(), WorldY(), m_OriginCol, m_OriginRow);
		m_RoamTargetCol = m_OriginCol;
		m_RoamTargetRow = m_OriginRow;
		m_GroundHeight = WorldGrid::GroundHeightAt(WorldX(), WorldY());
		m_Anchored = true;
	}

	// Sparks in the colour of whatever form he struck with.
	if (m_SparksPending)
	{
		SparkActor::Style style = SparkActor::StyleFor(ctx.status->form);
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), style, kHitSparks, kHitSparkSpeed, kHitSparkLift);
		m_SparksPending = false;
	}

	if (m_Health <= 0.f)
	{
		SpawnDeathBurst(ctx);
		Destroy();
		return;
	}

	m_Time += ctx.deltaTime;
	m_Moving = false;
	if (m_AttackCooldown > 0.f)
	{
		m_AttackCooldown -= ctx.deltaTime;
	}
	if (m_SpecialCooldown > 0.f)
	{
		m_SpecialCooldown -= ctx.deltaTime;
	}
	if (m_StrikeFlash > 0.f)
	{
		m_StrikeFlash -= ctx.deltaTime / kStrikeShowTime;
	}
	if (m_HitFlash > 0.f)
	{
		m_HitFlash -= ctx.deltaTime / kHitFlashTime;
	}

	m_GroundHeight = WorldGrid::SmoothApproach(m_GroundHeight,
		WorldGrid::GroundHeightAt(WorldX(), WorldY()), kClimbRate, ctx.deltaTime);

	if (m_ShoveTimer > 0.f)
	{
		// The last frame covers only what is left, so every shove travels its
		// full distance and no further.
		float time = (ctx.deltaTime < m_ShoveTimer) ? ctx.deltaTime : m_ShoveTimer;

		float col, row, offsetCol, offsetRow;
		WorldGrid::InverseIso(LocalX(), LocalY(), col, row);
		WorldGrid::InverseIso(m_ShoveVelX * time, m_ShoveVelY * time, offsetCol, offsetRow);
		WorldGrid::Move(col, row, offsetCol, offsetRow);

		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		SetLocalPosition(x, y);

		m_ShoveTimer -= ctx.deltaTime;
		return;
	}

	// Ranges are judged on the ground, in tiles, so its reach is the same
	// whether the player stands beside it on screen or above it.
	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float toCol = playerCol - col;
	float toRow = playerRow - row;
	float distance = sqrtf(toCol * toCol + toRow * toRow);
	float dirCol = (distance > 0.001f) ? toCol / distance : m_FaceCol;
	float dirRow = (distance > 0.001f) ? toRow / distance : m_FaceRow;

	m_StateTime += ctx.deltaTime;

	switch (m_State)
	{
	case State::Roam:
	{
		if (distance <= Tuning::Monster::kDetectRange)
		{
			m_State = State::Chase;
			m_StateTime = 0.f;
		}
		else
		{
			Roam(ctx);
		}
		break;
	}
	case State::Chase:
	{
		UpdateChase(ctx, distance, dirCol, dirRow);
		break;
	}
	case State::Windup:
	{
		// A dark mage keeps drifting while it casts; everything else stands
		// to gather a blow.
		if (IsMage(m_Kind) && !IsLight(m_Alignment))
		{
			Strafe(ctx, dirCol, dirRow);
		}

		if (m_StateTime >= m_BlowWindup)
		{
			Release(ctx);
		}
		break;
	}
	case State::Charge:
	{
		UpdateCharge(ctx);
		break;
	}
	case State::Recover:
	{
		UpdateRecover(ctx);
		break;
	}
	}
}

void MonsterActor::UpdateChase(const ActorContext& ctx, float distance, float dirCol, float dirRow)
{
	if (distance > Tuning::Monster::kGiveUpRange)
	{
		m_State = State::Roam;
		m_StateTime = 0.f;
		m_RoamTargetCol = m_OriginCol;
		m_RoamTargetRow = m_OriginRow;
		return;
	}

	FaceAlong(dirCol, dirRow);

	if (TryStartBlow(distance, dirCol, dirRow))
	{
		return;
	}

	if (IsMage(m_Kind))
	{
		KeepDistance(ctx, distance, dirCol, dirRow);
		return;
	}

	float hold = StartRange(m_Alignment) * kHoldShare;
	if (distance <= hold)
	{
		return;
	}

	float step = Tuning::Monster::kChaseSpeed * SpeedScale(m_Kind, m_Alignment) * ctx.deltaTime;
	if (step > distance - hold)
	{
		step = distance - hold;
	}
	StepAlong(dirCol, dirRow, step);
}

void MonsterActor::KeepDistance(const ActorContext& ctx, float distance, float dirCol, float dirRow)
{
	float preferred = PreferredRange(m_Alignment);
	float speed = Tuning::Monster::kChaseSpeed * SpeedScale(m_Kind, m_Alignment);

	// Too close: back away, still facing him.
	if (distance < preferred - Tuning::Monster::kMageBackOff)
	{
		StepAlong(-dirCol, -dirRow, speed * ctx.deltaTime);
		return;
	}

	// Too far: close in, but only to the distance it likes.
	if (distance > preferred)
	{
		float step = speed * ctx.deltaTime;
		if (step > distance - preferred)
		{
			step = distance - preferred;
		}
		StepAlong(dirCol, dirRow, step);
		return;
	}

	if (!IsLight(m_Alignment))
	{
		Strafe(ctx, dirCol, dirRow);
	}
}

void MonsterActor::Strafe(const ActorContext& ctx, float dirCol, float dirRow)
{
	m_StrafeTimer -= ctx.deltaTime;
	if (m_StrafeTimer <= 0.f)
	{
		m_StrafeSign = -m_StrafeSign;
		m_StrafeTimer = Tuning::DarkMage::kStrafeSwitchMin
			+ Random::Unit() * (Tuning::DarkMage::kStrafeSwitchMax - Tuning::DarkMage::kStrafeSwitchMin);
	}

	float speed = Tuning::Monster::kChaseSpeed * SpeedScale(m_Kind, m_Alignment) * Tuning::DarkMage::kStrafeScale;
	StepAlong(-dirRow * m_StrafeSign, dirCol * m_StrafeSign, speed * ctx.deltaTime);
}

bool MonsterActor::TryStartBlow(float distance, float dirCol, float dirRow)
{
	if (IsMage(m_Kind))
	{
		return TryStartMageBlow(distance, dirCol, dirRow);
	}
	return TryStartKnightBlow(distance, dirCol, dirRow);
}

bool MonsterActor::TryStartMageBlow(float distance, float dirCol, float dirRow)
{
	if (distance > PreferredRange(m_Alignment) + Tuning::Monster::kMageCastSlack)
	{
		return false;
	}

	bool light = IsLight(m_Alignment);
	if (m_SpecialCooldown <= 0.f)
	{
		if (light)
		{
			StartWindup(Blow::Smite, Tuning::LightMage::kSmiteWindup, dirCol, dirRow);
			m_SpecialCooldown = Tuning::LightMage::kSmiteCooldown;
		}
		else
		{
			StartWindup(Blow::Curse, Tuning::DarkMage::kCurseWindup, dirCol, dirRow);
			m_SpecialCooldown = Tuning::DarkMage::kCurseCooldown;
		}
		return true;
	}

	if (m_AttackCooldown > 0.f)
	{
		return false;
	}

	if (light)
	{
		StartWindup(Blow::Bolt, Tuning::LightMage::kBoltWindup, dirCol, dirRow);
	}
	else
	{
		StartWindup(Blow::Orb, Tuning::DarkMage::kOrbWindup, dirCol, dirRow);
	}
	return true;
}

bool MonsterActor::TryStartKnightBlow(float distance, float dirCol, float dirRow)
{
	if (!IsLight(m_Alignment) && m_SpecialCooldown <= 0.f
		&& distance >= Tuning::DarkKnight::kChargeMinRange && distance <= Tuning::DarkKnight::kChargeMaxRange)
	{
		StartWindup(Blow::Charge, Tuning::DarkKnight::kChargeWindup, dirCol, dirRow);
		m_SpecialCooldown = Tuning::DarkKnight::kChargeCooldown;
		return true;
	}

	if (m_AttackCooldown > 0.f || distance > StartRange(m_Alignment))
	{
		return false;
	}

	if (IsLight(m_Alignment))
	{
		StartWindup(Blow::Cleave, Tuning::LightKnight::kCleaveWindup, dirCol, dirRow);
	}
	else
	{
		StartWindup(Blow::TwinCut, Tuning::DarkKnight::kCutWindup, dirCol, dirRow);
	}
	return true;
}

void MonsterActor::StartWindup(Blow blow, float windup, float dirCol, float dirRow)
{
	m_Blow = blow;
	m_BlowWindup = windup;
	m_BlowDirCol = dirCol;
	m_BlowDirRow = dirRow;
	m_State = State::Windup;
	m_StateTime = 0.f;
	FaceAlong(dirCol, dirRow);
}

void MonsterActor::Release(const ActorContext& ctx)
{
	switch (m_Blow)
	{
	case Blow::Cleave:
	{
		Combat::StrikePlayerArc(ctx, WorldX(), WorldY(), m_BlowDirCol, m_BlowDirRow,
			Tuning::LightKnight::kCleaveRange, Tuning::LightKnight::kCleaveArcDegrees,
			Tuning::LightKnight::kCleaveDamage);
		m_StrikeFlash = 1.f;
		m_AttackCooldown = Tuning::LightKnight::kCleaveCooldown;
		m_State = State::Recover;
		break;
	}
	case Blow::TwinCut:
	{
		Cut(ctx);
		m_SecondCutPending = true;
		m_AttackCooldown = Tuning::DarkKnight::kCutCooldown;
		m_State = State::Recover;
		break;
	}
	case Blow::Charge:
	{
		m_ChargeLanded = false;
		m_State = State::Charge;
		break;
	}
	case Blow::Bolt:
	case Blow::Smite:
	case Blow::Orb:
	case Blow::Curse:
	{
		Cast(ctx);
		m_State = State::Recover;
		break;
	}
	}

	m_StateTime = 0.f;
}

void MonsterActor::Cast(const ActorContext& ctx)
{
	// Thrown spells are aimed where he is now, not where he was when the
	// casting began - the casting itself is the warning.
	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float toCol = playerCol - col;
	float toRow = playerRow - row;
	float length = sqrtf(toCol * toCol + toRow * toRow);
	float dirCol = (length > 0.001f) ? toCol / length : m_BlowDirCol;
	float dirRow = (length > 0.001f) ? toRow / length : m_BlowDirRow;
	FaceAlong(dirCol, dirRow);

	std::unique_ptr<Actor> spell;
	float spawnX = WorldX();
	float spawnY = WorldY();

	switch (m_Blow)
	{
	case Blow::Bolt:
	{
		spell.reset(new EnemyProjectileActor(Alignment::Light, dirCol, dirRow,
			Tuning::LightMage::kBoltSpeed, 0.f, Tuning::LightMage::kBoltLifetime, Tuning::LightMage::kBoltDamage));
		m_AttackCooldown = Tuning::LightMage::kBoltCooldown;
		break;
	}
	case Blow::Orb:
	{
		spell.reset(new EnemyProjectileActor(Alignment::Dark, dirCol, dirRow,
			Tuning::DarkMage::kOrbSpeed, Tuning::DarkMage::kOrbTurnRate, Tuning::DarkMage::kOrbLifetime,
			Tuning::DarkMage::kOrbDamage));
		m_AttackCooldown = Tuning::DarkMage::kOrbCooldown;
		break;
	}
	case Blow::Smite:
	{
		spell.reset(new LightningActor(LightningActor::Target::Player,
			Tuning::LightMage::kSmiteRadius, Tuning::LightMage::kSmiteDelay, Tuning::LightMage::kSmiteDamage));
		spawnX = ctx.playerX;
		spawnY = ctx.playerY;
		break;
	}
	default:
	{
		spell.reset(new CurseFieldActor());
		spawnX = ctx.playerX;
		spawnY = ctx.playerY;
		break;
	}
	}

	spell->SetLocalPosition(spawnX, spawnY);
	ctx.graph->Spawn(std::move(spell));
	m_StrikeFlash = 1.f;
}

void MonsterActor::Cut(const ActorContext& ctx)
{
	Combat::StrikePlayerArc(ctx, WorldX(), WorldY(), m_BlowDirCol, m_BlowDirRow,
		Tuning::DarkKnight::kCutRange, Tuning::DarkKnight::kCutArcDegrees,
		Tuning::DarkKnight::kCutDamage);
	m_StrikeFlash = 1.f;
}

void MonsterActor::UpdateCharge(const ActorContext& ctx)
{
	// Only the part of this frame that falls inside the charge moves it, so
	// it always covers exactly the marked line.
	float before = m_StateTime - ctx.deltaTime;
	if (before < 0.f)
	{
		before = 0.f;
	}
	float until = (m_StateTime < Tuning::DarkKnight::kChargeDuration) ? m_StateTime : Tuning::DarkKnight::kChargeDuration;
	float speed = Tuning::DarkKnight::kChargeDistance / Tuning::DarkKnight::kChargeDuration;
	if (until > before)
	{
		StepAlong(m_BlowDirCol, m_BlowDirRow, speed * (until - before));
	}

	if (!m_ChargeLanded && Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(),
		Tuning::DarkKnight::kChargeHitRadius, Tuning::DarkKnight::kChargeDamage))
	{
		m_ChargeLanded = true;
		m_StrikeFlash = 1.f;
	}

	if (m_StateTime >= Tuning::DarkKnight::kChargeDuration)
	{
		m_State = State::Recover;
		m_StateTime = 0.f;
	}
}

void MonsterActor::UpdateRecover(const ActorContext& ctx)
{
	if (m_SecondCutPending)
	{
		if (m_StateTime >= Tuning::DarkKnight::kSecondCutGap)
		{
			Cut(ctx);
			m_SecondCutPending = false;
			m_StateTime = 0.f;
		}
		return;
	}

	if (m_StateTime >= RecoverTime(m_Kind, m_Alignment))
	{
		m_State = State::Chase;
		m_StateTime = 0.f;
	}
}

void MonsterActor::Roam(const ActorContext& ctx)
{
	float col, row;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);

	float toCol = m_RoamTargetCol - col;
	float toRow = m_RoamTargetRow - row;
	if (toCol * toCol + toRow * toRow > kRoamArrive * kRoamArrive)
	{
		StepToward(m_RoamTargetCol, m_RoamTargetRow,
			Tuning::Monster::kRoamSpeed * SpeedScale(m_Kind, m_Alignment), ctx.deltaTime);
		return;
	}

	if (m_RoamWaitTimer > 0.f)
	{
		m_RoamWaitTimer -= ctx.deltaTime;
		return;
	}

	float angle = Random::Angle();
	float radius = Random::Unit() * Tuning::Monster::kRoamRadius;
	m_RoamTargetCol = m_OriginCol + cosf(angle) * radius;
	m_RoamTargetRow = m_OriginRow + sinf(angle) * radius;
	WorldGrid::ClampToMap(m_RoamTargetCol, m_RoamTargetRow);
	m_RoamWaitTimer = 1.5f + Random::Unit() * 2.f;
}

void MonsterActor::StepToward(float targetCol, float targetRow, float speed, float deltaTime)
{
	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);

	float toCol = targetCol - col;
	float toRow = targetRow - row;
	float distance = sqrtf(toCol * toCol + toRow * toRow);
	if (distance <= 0.001f)
	{
		return;
	}

	float step = speed * deltaTime;
	if (step > distance)
	{
		step = distance;
	}

	FaceAlong(toCol / distance, toRow / distance);
	StepAlong(toCol / distance, toRow / distance, step);
}

void MonsterActor::StepAlong(float dirCol, float dirRow, float tiles)
{
	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);

	WorldGrid::Move(col, row, dirCol * tiles, dirRow * tiles);

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);

	m_GaitPhase += tiles * kGaitPerTile;
	m_Moving = true;
}

void MonsterActor::FaceAlong(float dirCol, float dirRow)
{
	m_FaceCol = dirCol;
	m_FaceRow = dirRow;

	float screenX, screenY;
	WorldGrid::IsoPosition(dirCol, dirRow, screenX, screenY);
	if (fabsf(screenX) > 0.01f)
	{
		m_Facing = (screenX < 0.f) ? -1.f : 1.f;
	}
}

void MonsterActor::SpawnDeathBurst(const ActorContext& ctx) const
{
	for (int i = 0; i < kBurstCount; i++)
	{
		float angle = Random::Angle();
		float speed = kBurstSpeed * (0.5f + Random::Unit() * 0.5f);

		std::unique_ptr<ParticleActor> mote(
			new ParticleActor(m_Alignment, cosf(angle) * speed, sinf(angle) * speed));
		mote->SetLocalPosition(WorldX(), WorldY());

		ctx.graph->Spawn(std::move(mote));
	}
}

void MonsterActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	// Wide enough to keep a charge's marked path from popping at the edges.
	if (!ctx.IsOnScreen(x, y, WorldGrid::kTileWidth * 3.f, WorldGrid::kTileHeight * 4.f))
	{
		return;
	}

	float groundY = y + m_GroundHeight;

	// Only knights mark the ground; a mage's casting is its own warning, and
	// its spells mark their own ground where they need to.
	if (m_State == State::Windup && m_Kind == Kind::Knight)
	{
		DrawTelegraph(out, x, groundY);
	}

	MonsterModels::Pose pose;
	pose.x = x;
	pose.groundY = groundY;
	pose.depth = WorldGrid::TileDepth(WorldX(), WorldY());
	pose.worldX = WorldX();
	pose.worldY = WorldY();
	pose.facing = m_Facing;
	pose.gait = m_GaitPhase;
	pose.moving = m_Moving;
	pose.time = m_Time;
	pose.strike = (m_StrikeFlash > 0.f) ? m_StrikeFlash : 0.f;
	pose.charging = (m_State == State::Charge);
	if (m_State == State::Windup)
	{
		pose.windup = m_StateTime / m_BlowWindup;
		if (pose.windup > 1.f)
		{
			pose.windup = 1.f;
		}
	}

	if (m_HitFlash > 0.f)
	{
		out.SetFlash(m_HitFlash * kHitFlashStrength);
	}

	bool light = IsLight(m_Alignment);
	if (IsMage(m_Kind))
	{
		if (light)
		{
			MonsterModels::DrawLightMage(out, pose);
		}
		else
		{
			MonsterModels::DrawDarkMage(out, pose);
		}
	}
	else if (light)
	{
		MonsterModels::DrawLightKnight(out, pose);
	}
	else
	{
		MonsterModels::DrawDarkKnight(out, pose);
	}
}

void MonsterActor::DrawTelegraph(DrawList& out, float x, float groundY) const
{
	float progress = m_StateTime / m_BlowWindup;
	if (progress > 1.f)
	{
		progress = 1.f;
	}

	float r, g, b;
	AlignmentColor::Warning(m_Alignment, Palette::kWarningWash, r, g, b);
	float baseY = groundY + kTelegraphLift;

	if (m_Blow == Blow::Charge)
	{
		float length = Tuning::DarkKnight::kChargeDistance;
		float width = Tuning::DarkKnight::kChargeWidth;
		out.AddGroundStrip(WorldX(), WorldY(), x, baseY, m_BlowDirCol, m_BlowDirRow,
			0.f, length, width, r, g, b, kTelegraphBase);
		out.AddGroundStrip(WorldX(), WorldY(), x, baseY, m_BlowDirCol, m_BlowDirRow,
			0.f, length * progress, width, r, g, b, kTelegraphFill);
		return;
	}

	float range = (m_Blow == Blow::Cleave) ? Tuning::LightKnight::kCleaveRange : Tuning::DarkKnight::kCutRange;
	float arc = (m_Blow == Blow::Cleave) ? Tuning::LightKnight::kCleaveArcDegrees : Tuning::DarkKnight::kCutArcDegrees;
	float center = atan2f(m_BlowDirRow, m_BlowDirCol);
	float half = arc * 0.5f * kDegToRad;

	out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, range, center - half, center + half,
		kTelegraphSegments, r, g, b, kTelegraphBase);
	if (progress > 0.f)
	{
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, range * progress, center - half, center + half,
			kTelegraphSegments, r, g, b, kTelegraphFill);
	}
	out.AddGroundArc(WorldX(), WorldY(), x, baseY, range - kTelegraphEdgeWidth, range, center - half, center + half,
		kTelegraphSegments, r, g, b, kTelegraphEdge);
}
