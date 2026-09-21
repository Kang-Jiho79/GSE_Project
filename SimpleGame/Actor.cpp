#include "stdafx.h"
#include "Actor.h"
#include "DrawList.h"
#include <algorithm>

void ActorContext::ToScreen(float worldX, float worldY, float& outX, float& outY) const
{
	outX = worldX - camX;
	outY = worldY - camY;
}

bool ActorContext::IsOnScreen(float screenX, float screenY, float padX, float padY) const
{
	float halfW = windowSizeX / 2.f;
	float halfH = windowSizeY / 2.f;
	return screenX > -halfW - padX && screenX < halfW + padX
		&& screenY > -halfH - padY && screenY < halfH + padY;
}

Actor::Actor()
{
}

Actor::~Actor()
{
}

Actor* Actor::AddChild(std::unique_ptr<Actor> child)
{
	Actor* raw = child.get();
	m_Children.push_back(std::move(child));
	return raw;
}

void Actor::SetLocalPosition(float x, float y)
{
	m_LocalX = x;
	m_LocalY = y;
}

float Actor::LocalX() const
{
	return m_LocalX;
}

float Actor::LocalY() const
{
	return m_LocalY;
}

float Actor::WorldX() const
{
	return m_WorldX;
}

float Actor::WorldY() const
{
	return m_WorldY;
}

void Actor::Destroy()
{
	m_Destroyed = true;
}

bool Actor::IsDestroyed() const
{
	return m_Destroyed;
}

void Actor::UpdateTree(const ActorContext& ctx, float parentWorldX, float parentWorldY)
{
	if (m_Destroyed)
	{
		return;
	}

	// Resolved before OnUpdate so an actor can read where it currently is, and
	// again after, so anything querying it this frame sees where it moved to.
	m_WorldX = parentWorldX + m_LocalX;
	m_WorldY = parentWorldY + m_LocalY;

	OnUpdate(ctx);

	m_WorldX = parentWorldX + m_LocalX;
	m_WorldY = parentWorldY + m_LocalY;

	for (size_t i = 0; i < m_Children.size(); i++)
	{
		m_Children[i]->UpdateTree(ctx, m_WorldX, m_WorldY);
	}
}

void Actor::DrawTree(DrawList& out, const ActorContext& ctx, float parentWorldX, float parentWorldY) const
{
	if (m_Destroyed)
	{
		return;
	}

	// Re-resolved rather than trusted, so an actor spawned after the last
	// update still draws in the right place.
	m_WorldX = parentWorldX + m_LocalX;
	m_WorldY = parentWorldY + m_LocalY;

	OnDraw(out, ctx);

	for (size_t i = 0; i < m_Children.size(); i++)
	{
		m_Children[i]->DrawTree(out, ctx, m_WorldX, m_WorldY);
	}
}

void Actor::PruneDestroyed()
{
	m_Children.erase(
		std::remove_if(m_Children.begin(), m_Children.end(), [](const std::unique_ptr<Actor>& child)
		{
			return child->IsDestroyed();
		}),
		m_Children.end());

	for (size_t i = 0; i < m_Children.size(); i++)
	{
		m_Children[i]->PruneDestroyed();
	}
}

void Actor::OnUpdate(const ActorContext& ctx)
{
}

void Actor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
}
