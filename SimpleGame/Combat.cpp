#include "stdafx.h"
#include "Combat.h"
#include "SceneGraph.h"
#include "MonsterActor.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;
}

float Combat::BoltDamage()
{
	return Tuning::LightForm::kBoltDamage;
}

float Combat::SlashDamage(int comboStep)
{
	return Tuning::DarkForm::kSlashDamage[comboStep];
}

void Combat::HitArc(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
	float rangeTiles, float arcDegrees, float damage)
{
	float originCol, originRow;
	WorldGrid::InverseIso(originX, originY, originCol, originRow);

	float minDot = cosf(arcDegrees * 0.5f * kDegToRad);

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

		monster.TakeHit(damage);
	});
}
