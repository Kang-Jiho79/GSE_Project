#include "stdafx.h"
#include "Combat.h"
#include "Actor.h"
#include "SceneGraph.h"
#include "MonsterActor.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;

	float LightPower(const PlayerStatus& status)
	{
		return 1.f + status.lightTotal * Tuning::Power::kScalePerAbsorbed;
	}

	float DarkPower(const PlayerStatus& status)
	{
		return 1.f + status.darkTotal * Tuning::Power::kScalePerAbsorbed;
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

float Combat::Lifesteal(float damageLanded, float scale)
{
	return damageLanded * Tuning::DarkForm::kLifesteal * scale;
}

float Combat::HitArc(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
	float rangeTiles, float arcDegrees, float damage)
{
	float originCol, originRow;
	WorldGrid::InverseIso(originX, originY, originCol, originRow);

	float minDot = cosf(arcDegrees * 0.5f * kDegToRad);
	float landed = 0.f;

	graph.ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		float col, row;
		WorldGrid::InverseIso(monster.WorldX(), monster.WorldY(), col, row);

		float toCol = col - originCol;
		float toRow = row - originRow;
		float distance = sqrtf(toCol * toCol + toRow * toRow);
		if (distance > rangeTiles)
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

		landed += monster.TakeHit(damage);
	});

	return landed;
}

float Combat::HitRadius(SceneGraph& graph, float centerX, float centerY,
	float radiusTiles, float damage, float shoveTiles)
{
	float centerCol, centerRow;
	WorldGrid::InverseIso(centerX, centerY, centerCol, centerRow);

	float landed = 0.f;

	graph.ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		float col, row;
		WorldGrid::InverseIso(monster.WorldX(), monster.WorldY(), col, row);

		float fromCol = col - centerCol;
		float fromRow = row - centerRow;
		float distance = sqrtf(fromCol * fromCol + fromRow * fromRow);
		if (distance > radiusTiles)
		{
			return;
		}

		if (damage > 0.f)
		{
			landed += monster.TakeHit(damage);
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

		monster.Shove(fromCol / distance * amount, fromRow / distance * amount);
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

	graph.ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		float col, row;
		WorldGrid::InverseIso(monster.WorldX(), monster.WorldY(), col, row);

		float toCol = col - originCol;
		float toRow = row - originRow;

		float along = toCol * dirCol + toRow * dirRow;
		if (along < 0.f || along > lengthTiles)
		{
			return;
		}

		float across = fabsf(toCol * dirRow - toRow * dirCol);
		if (across > halfWidth)
		{
			return;
		}

		if (blockable)
		{
			landed += monster.TakeHit(damage, originX, originY);
		}
		else
		{
			landed += monster.TakeHit(damage);
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

	return ctx.status->TryDamage(damage);
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

	return ctx.status->TryDamage(damage);
}
