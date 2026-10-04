#include "stdafx.h"
#include "SceneGraph.h"
#include "DrawList.h"

SceneGraph::SceneGraph()
	: m_SpawnParent(&m_Root)
{
}

Actor& SceneGraph::Root()
{
	return m_Root;
}

void SceneGraph::SetSpawnParent(Actor* parent)
{
	if (parent != nullptr)
	{
		m_SpawnParent = parent;
	}
}

void SceneGraph::Spawn(std::unique_ptr<Actor> actor)
{
	m_Pending.push_back(std::move(actor));
}

void SceneGraph::Clear()
{
	m_Root.RemoveAllChildren();
	m_Pending.clear();
	m_SpawnParent = &m_Root;
}

void SceneGraph::Update(const ActorContext& ctx)
{
	m_Root.UpdateTree(ctx, 0.f, 0.f);

	IntegratePending();
	m_Root.PruneDestroyed();
}

void SceneGraph::Draw(DrawList& out, const ActorContext& ctx)
{
	m_Root.DrawTree(out, ctx, 0.f, 0.f);
}

void SceneGraph::IntegratePending()
{
	for (size_t i = 0; i < m_Pending.size(); i++)
	{
		m_SpawnParent->AddChild(std::move(m_Pending[i]));
	}

	m_Pending.clear();
}
