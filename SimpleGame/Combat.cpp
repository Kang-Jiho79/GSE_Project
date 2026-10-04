#include "stdafx.h"
#include "Combat.h"
#include "Actor.h"
#include "SceneGraph.h"
#include "EnemyActor.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include "Impact.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;

	// Lucius's blessing makes every light blow that much harder on top.
	float LightPower(const PlayerStatus& status)
	{
		float blessing = status.lightBlessing ? 1.f + Tuning::Duel::kLightBlessing : 1.f;
		return (1.f + status.lightTotal * Tuning::Power::kScalePerAbsorbed) * blessing;
	}

	float DarkPower(const PlayerStatus& status)
	{
		return 1.f + status.darkTotal * Tuning::Power::kScalePerAbsorbed;
	}

	float AshPower(const PlayerStatus& status)
	{
		return 1.f + (status.lightTotal + status.darkTotal) * 0.5f * Tuning::Power::kScalePerAbsorbed;
	}

	// A blow that gets through to him is felt: the world catches and the
	// screen jolts.
	bool HurtPlayer(const ActorContext& ctx, float damage)
	{
		if (!ctx.status->TryDamage(damage))
		{
			return false;
		}

		ctx.impact->HitStop(Tuning::Feel::kHurtStop);
		ctx.impact->Shake(Tuning::Feel::kHurtShake);
		return true;
	}
}

float Combat::BoltDamage(const PlayerStatus& status)
{
	return Tuning::LightForm::kBoltDamage * LightPower(status);
}

float Combat::BeamDamage(const PlayerStatus& status)
{
	return Tuning::LightForm::kBeamDamage * LightPower(status);
}

float Combat::StrikeDamage(const PlayerStatus& status)
{
	return Tuning::LightForm::kStrikeDamage * LightPower(status);
}

float Combat::SlashDamage(const PlayerStatus& status, int comboStep)
{
	return Tuning::DarkForm::kSlashDamage[comboStep] * DarkPower(status);
}

float Combat::DashDamage(const PlayerStatus& status)
{
	return Tuning::DarkForm::kDashDamage * DarkPower(status);
}

float Combat::SpinDamage(const PlayerStatus& status)
{
	return Tuning::DarkForm::kSpinDamage * DarkPower(status);
}

float Combat::DarkBurstDamage(const PlayerStatus& status)
{
	return Tuning::SwapSkill::kDarkBurstDamage * DarkPower(status);
}

float Combat::AshWaveDamage(const PlayerStatus& status)
{
	return Tuning::AshForm::kWaveDamage * AshPower(status);
}

float Combat::UnravelDamage(const PlayerStatus& status)
{
	return Tuning::AshForm::kUnravelDamage * AshPower(status);
}

float Combat::EraseDamage(const PlayerStatus& status)
{
	return Tuning::AshForm::kEraseDamage * AshPower(status);
}

float Combat::Lifesteal(const PlayerStatus& status, float damageLanded, float scale)
{
	float share = Tuning::DarkForm::kLifesteal + (status.darkBlessing ? Tuning::Duel::kLifestealBlessing : 0.f);
	return damageLanded * share * scale;
}

float Combat::HitArc(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
	float rangeTiles, float arcDegrees, float damage)
{
	float originCol, originRow;
	WorldGrid::InverseIso(originX, originY, originCol, originRow);

	float minDot = cosf(arcDegrees * 0.5f * kDegToRad);
	float landed = 0.f;

	graph.ForEach<EnemyActor>([&](EnemyActor& enemy)
	{
		float col, row;
		WorldGrid::InverseIso(enemy.WorldX(), enemy.WorldY(), col, row);

		float toCol = col - originCol;
		float toRow = row - originRow;
		float distance = sqrtf(toCol * toCol + toRow * toRow);
		if (distance > rangeTiles + enemy.BodyRadius())
		{
			return;
		}

		if (distance > Tuning::DarkForm::kPointBlank)
		{
			float dot = (toCol * dirCol + toRow * dirRow) / distance;
			if (dot < minDot)
			{
				return;
			}
		}

		landed += enemy.TakeHit(damage);
	});

	return landed;
}

float Combat::HitRadius(SceneGraph& graph, float centerX, float centerY,
	float radiusTiles, float damage, float shoveTiles)
{
	float centerCol, centerRow;
	WorldGrid::InverseIso(centerX, centerY, centerCol, centerRow);

	float landed = 0.f;

	graph.ForEach<EnemyActor>([&](EnemyActor& enemy)
	{
		float col, row;
		WorldGrid::InverseIso(enemy.WorldX(), enemy.WorldY(), col, row);

		float fromCol = col - centerCol;
		float fromRow = row - centerRow;
		float distance = sqrtf(fromCol * fromCol + fromRow * fromRow);
		if (distance > radiusTiles + enemy.BodyRadius())
		{
			return;
		}

		if (damage > 0.f)
		{
			landed += enemy.TakeHit(damage);
		}

		// Right on the centre there is no direction to shove it in.
		if (shoveTiles == 0.f || distance < 0.001f)
		{
			return;
		}

		float amount = shoveTiles;
		if (shoveTiles < 0.f)
		{
			float room = distance - Tuning::Shove::kPullStop;
			if (room < 0.f)
			{
				room = 0.f;
			}
			amount = (-shoveTiles < room) ? shoveTiles : -room;
		}

		enemy.Shove(fromCol / distance * amount, fromRow / distance * amount);
	});

	return landed;
}

float Combat::HitLine(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
	float lengthTiles, float widthTiles, float damage, bool blockable)
{
	float originCol, originRow;
	WorldGrid::InverseIso(originX, originY, originCol, originRow);

	float halfWidth = widthTiles * 0.5f;
	float landed = 0.f;

	graph.ForEach<EnemyActor>([&](EnemyActor& enemy)
	{
		float col, row;
		WorldGrid::InverseIso(enemy.WorldX(), enemy.WorldY(), col, row);

		float toCol = col - originCol;
		float toRow = row - originRow;

		float body = enemy.BodyRadius();
		float along = toCol * dirCol + toRow * dirRow;
		if (along < -body || along > lengthTiles + body)
		{
			return;
		}

		float across = fabsf(toCol * dirRow - toRow * dirCol);
		if (across > halfWidth + body)
		{
			return;
		}

		enemy.Pierce();
		if (blockable)
		{
			landed += enemy.TakeHit(damage, originX, originY);
		}
		else
		{
			landed += enemy.TakeHit(damage);
		}
	});

	return landed;
}

bool Combat::StrikePlayerArc(const ActorContext& ctx, float originX, float originY, float dirCol, float dirRow,
	float rangeTiles, float arcDegrees, float damage)
{
	float originCol, originRow, playerCol, playerRow;
	WorldGrid::InverseIso(originX, originY, originCol, originRow);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);

	float toCol = playerCol - originCol;
	float toRow = playerRow - originRow;
	float distance = sqrtf(toCol * toCol + toRow * toRow);
	if (distance > rangeTiles)
	{
		return false;
	}

	if (distance > Tuning::DarkForm::kPointBlank)
	{
		float dot = (toCol * dirCol + toRow * dirRow) / distance;
		if (dot < cosf(arcDegrees * 0.5f * kDegToRad))
		{
			return false;
		}
	}

	return HurtPlayer(ctx, damage);
}

bool Combat::StrikePlayerRadius(const ActorContext& ctx, float centerX, float centerY,
	float radiusTiles, float damage)
{
	float centerCol, centerRow, playerCol, playerRow;
	WorldGrid::InverseIso(centerX, centerY, centerCol, centerRow);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);

	float toCol = playerCol - centerCol;
	float toRow = playerRow - centerRow;
	if (toCol * toCol + toRow * toRow > radiusTiles * radiusTiles)
	{
		return false;
	}

	return HurtPlayer(ctx, damage);
}

bool Combat::StrikePlayerLine(const ActorContext& ctx, float originX, float originY, float dirCol, float dirRow,
	float lengthTiles, float widthTiles, float damage)
{
	float originCol, originRow, playerCol, playerRow;
	WorldGrid::InverseIso(originX, originY, originCol, originRow);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);

	float toCol = playerCol - originCol;
	float toRow = playerRow - originRow;

	float along = toCol * dirCol + toRow * dirRow;
	if (along < 0.f || along > lengthTiles)
	{
		return false;
	}
	if (fabsf(toCol * dirRow - toRow * dirCol) > widthTiles * 0.5f)
	{
		return false;
	}

	return HurtPlayer(ctx, damage);
}
