#include "stdafx.h"
#include "Prologue.h"
#include "Dialogue.h"
#include "Progress.h"
#include "SceneGraph.h"
#include "Impact.h"
#include "WorldGrid.h"
#include "MonsterActor.h"
#include "NpcActor.h"
#include "SparkActor.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const int kDustSparks = 14;
	const float kDustSpeed = 120.f;
	const float kDustLift = 30.f;
}

void Prologue::Begin()
{
	m_Stage = Stage::Opening;
	m_Time = 0.f;
	m_Taken = false;
	m_Left = 0;
}

void Prologue::Stop()
{
	m_Stage = Stage::Idle;
}

bool Prologue::IsActive() const
{
	return m_Stage != Stage::Idle;
}

bool Prologue::HoldsWaystone() const
{
	return IsActive();
}

int Prologue::RaidersLeft() const
{
	return (m_Stage == Stage::Raid) ? m_Left : -1;
}

int Prologue::Update(float deltaTime, SceneGraph& graph, Progress& progress, Impact& impact,
	float centerCol, float centerRow)
{
	switch (m_Stage)
	{
	case Stage::Opening:
	{
		m_Stage = Stage::Calm;
		m_Time = 0.f;
		return Dialogue::PrologueOpening;
	}
	case Stage::Calm:
	{
		m_Time += deltaTime;
		if (m_Time < Tuning::Prologue::kCalmTime)
		{
			return -1;
		}
		SpawnRaiders(graph, centerCol, centerRow);
		impact.Shake(Tuning::Prologue::kRaidShake);
		m_Stage = Stage::Raid;
		m_Time = 0.f;
		return Dialogue::PrologueRaid;
	}
	case Stage::Raid:
	{
		m_Time += deltaTime;
		if (!m_Taken && m_Time >= Tuning::Prologue::kTakeTime)
		{
			TakeVillagers(graph);
			m_Taken = true;
		}

		m_Left = 0;
		graph.ForEach<MonsterActor>([&](MonsterActor& monster)
		{
			if (!monster.IsDestroyed())
			{
				m_Left++;
			}
		});
		if (m_Taken && m_Left == 0)
		{
			m_Stage = Stage::Aftermath;
			m_Time = 0.f;
		}
		return -1;
	}
	case Stage::Aftermath:
	{
		m_Time += deltaTime;
		if (m_Time < Tuning::Prologue::kAftermathDelay)
		{
			return -1;
		}
		progress.Set(Story::VillageRaided);
		m_Stage = Stage::Idle;
		return Dialogue::PrologueAftermath;
	}
	default:
	{
		return -1;
	}
	}
}

void Prologue::SpawnRaiders(SceneGraph& graph, float centerCol, float centerRow)
{
	// In from every side at once, of both the old powers, a few of them mages.
	int count = Tuning::Prologue::kRaiders;
	for (int i = 0; i < count; i++)
	{
		float angle = i * 2.f * 3.14159265f / count;
		float col = centerCol + cosf(angle) * Tuning::Prologue::kRaidRing;
		float row = centerRow + sinf(angle) * Tuning::Prologue::kRaidRing;
		WorldGrid::ClampToMap(col, row);
		if (WorldGrid::IsBlocked(col, row))
		{
			col = centerCol + cosf(angle) * (Tuning::Prologue::kRaidRing - 2.f);
			row = centerRow + sinf(angle) * (Tuning::Prologue::kRaidRing - 2.f);
		}

		Alignment alignment = (i % 2 == 0) ? Alignment::Dark : Alignment::Light;
		MonsterActor::Kind kind = (i % 3 == 2) ? MonsterActor::Kind::Mage : MonsterActor::Kind::Knight;
		std::unique_ptr<MonsterActor> raider(new MonsterActor(kind, alignment));
		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		raider->SetLocalPosition(x, y);
		graph.Spawn(std::move(raider));
	}
}

void Prologue::TakeVillagers(SceneGraph& graph)
{
	graph.ForEach<NpcActor>([&](NpcActor& villager)
	{
		if (villager.IsDestroyed())
		{
			return;
		}
		SparkActor::Burst(graph, villager.WorldX(), villager.WorldY(), SparkActor::Style::Dust,
			kDustSparks, kDustSpeed, kDustLift);
		villager.Destroy();
	});
}
