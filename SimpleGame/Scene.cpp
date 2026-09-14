#include "stdafx.h"
#include "Scene.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
	const int kMapCols = 120;
	const int kMapRows = 120;

	const float kTileWidth = 64.f;
	const float kTileHeight = 32.f;
	const float kMoveSpeed = 5.f; // tiles per second

	const float kTerraceStep = 11.f; // pixels of rise per terrace level
	const int kTerraceLevels = 4;
	const float kNoiseCell = 9.f;

	// A face turned away from the light is darker. One light direction for the
	// whole world is what makes the boxes read as solid.
	const float kTopLight = 1.f;
	const float kRightLight = 0.70f;
	const float kLeftLight = 0.44f;

	const int kMonsterCount = 24;
	const float kMonsterMaxHealth = 3.f;
	const float kMonsterDetectRadius = 200.f;
	const float kMonsterGiveUpRadius = 300.f;
	const float kMonsterAttackRadius = 34.f;
	const float kMonsterChaseSpeed = 90.f; // pixels per second
	const float kMonsterRoamSpeed = 36.f;
	const float kMonsterRoamRadius = 70.f;
	const float kMonsterAttackCooldown = 1.2f;
	const float kMonsterAttackDamage = 8.f;

	const float kAttackCooldown = 0.3f;
	const float kProjectileSpeed = 420.f; // pixels per second
	const float kProjectileLifetime = 1.1f;
	const float kProjectileHitRadius = 20.f;

	const int kParticleBurstCount = 5;
	const float kParticleBurstSpeed = 90.f;
	const float kParticleFriction = 3.f; // velocity decay per second
	const float kParticleMagnetRadius = 90.f;
	const float kParticleMagnetAccel = 260.f;
	const float kParticleAbsorbRadius = 22.f;
	const float kParticleLifetime = 20.f;

	const float kPlayerInvulnDuration = 0.4f;

	const float kPlayerGlowRadius = 46.f;
	const float kProjectileGlowRadius = 26.f;

	const float kMinimapSize = 150.f;
	const float kMinimapBorder = 4.f;
	const float kMinimapRange = 42.f; // tiles from center to edge

	unsigned int Hash(int col, int row)
	{
		unsigned int h = (unsigned int)(col * 374761393 + row * 668265263 + 2166136261u);
		h = (h ^ (h >> 13)) * 1274126177u;
		return h ^ (h >> 16);
	}

	float HashUnit(int col, int row)
	{
		return (Hash(col, row) % 1000) / 1000.f;
	}

	float Clamp(float value, float low, float high)
	{
		if (value < low)
		{
			return low;
		}
		if (value > high)
		{
			return high;
		}
		return value;
	}

	// A random angle in radians, using the standard library's PRNG - fine here
	// because monsters and particles are simulated, stateful entities, unlike
	// the terrain and ruins below which must regenerate identically every frame.
	float RandomAngle()
	{
		return (rand() % 6283) / 1000.f;
	}

	float RandomUnit()
	{
		return (rand() % 1000) / 1000.f;
	}

	// Smoothed value noise quantized into terraces, so the ground has plateaus
	// and cliffs instead of per-tile static.
	float Elevation(int col, int row)
	{
		float fx = col / kNoiseCell;
		float fy = row / kNoiseCell;
		int x0 = (int)floorf(fx);
		int y0 = (int)floorf(fy);

		float tx = fx - x0;
		float ty = fy - y0;
		tx = tx * tx * (3.f - 2.f * tx);
		ty = ty * ty * (3.f - 2.f * ty);

		float v00 = HashUnit(x0, y0);
		float v10 = HashUnit(x0 + 1, y0);
		float v01 = HashUnit(x0, y0 + 1);
		float v11 = HashUnit(x0 + 1, y0 + 1);

		float top = v00 + (v10 - v00) * tx;
		float bottom = v01 + (v11 - v01) * tx;
		float noise = top + (bottom - top) * ty;

		int level = (int)(noise * kTerraceLevels);
		if (level < 0)
		{
			level = 0;
		}
		if (level > kTerraceLevels - 1)
		{
			level = kTerraceLevels - 1;
		}
		return level * kTerraceStep;
	}
}

Scene::Scene()
{
	SpawnMonsters();
}

void Scene::IsoPosition(float col, float row, float& outX, float& outY) const
{
	outX = (col - row) * (kTileWidth / 2.f);
	outY = -(col + row) * (kTileHeight / 2.f);
}

void Scene::InverseIso(float worldX, float worldY, float& outCol, float& outRow) const
{
	float u = worldX / (kTileWidth / 2.f);
	float v = -worldY / (kTileHeight / 2.f);
	outCol = (u + v) / 2.f;
	outRow = (v - u) / 2.f;
}

void Scene::WorldToTile(float worldX, float worldY, int& outCol, int& outRow) const
{
	float col, row;
	InverseIso(worldX, worldY, col, row);

	outCol = (int)(col + 0.5f);
	outRow = (int)(row + 0.5f);
	outCol = (int)Clamp((float)outCol, 0.f, (float)(kMapCols - 1));
	outRow = (int)Clamp((float)outRow, 0.f, (float)(kMapRows - 1));
}

float Scene::TileDepth(float worldX, float worldY) const
{
	int col, row;
	WorldToTile(worldX, worldY, col, row);
	return (float)(col + row);
}

bool Scene::IsOnScreen(float x, float y, float halfExtentX, float halfExtentY) const
{
	float halfW = m_WindowSizeX / 2.f;
	float halfH = m_WindowSizeY / 2.f;
	return x > -halfW - halfExtentX && x < halfW + halfExtentX
		&& y > -halfH - halfExtentY && y < halfH + halfExtentY;
}

int Scene::ComputeViewRadius() const
{
	float uMax = (m_WindowSizeX / 2.f) / (kTileWidth / 2.f) + 2.f;
	float vMax = (m_WindowSizeY / 2.f) / (kTileHeight / 2.f) + 4.f;
	return (int)std::ceil((uMax + vMax) / 2.f);
}

float Scene::GroundHeightAt(float worldX, float worldY) const
{
	int col, row;
	WorldToTile(worldX, worldY, col, row);
	return Elevation(col, row);
}

void Scene::Update(float screenRight, float screenUp, bool fireLight, bool fireDark,
	float mouseX, float mouseY, float deltaTime, PlayerStatus& status)
{
	// Movement: screen directions converted onto the isometric grid axes.
	float moveCol = screenRight - screenUp;
	float moveRow = -screenRight - screenUp;

	float lengthSquared = moveCol * moveCol + moveRow * moveRow;
	if (lengthSquared > 0.0001f)
	{
		float length = sqrtf(lengthSquared);
		m_PlayerCol += (moveCol / length) * kMoveSpeed * deltaTime;
		m_PlayerRow += (moveRow / length) * kMoveSpeed * deltaTime;

		m_PlayerCol = Clamp(m_PlayerCol, 0.f, (float)(kMapCols - 1));
		m_PlayerRow = Clamp(m_PlayerRow, 0.f, (float)(kMapRows - 1));
	}

	if (m_LightCooldown > 0.f)
	{
		m_LightCooldown -= deltaTime;
	}
	if (m_DarkCooldown > 0.f)
	{
		m_DarkCooldown -= deltaTime;
	}
	if (m_PlayerInvulnTimer > 0.f)
	{
		m_PlayerInvulnTimer -= deltaTime;
	}

	if (fireLight && m_LightCooldown <= 0.f)
	{
		FireProjectile(Alignment::Light, mouseX, mouseY);
		m_LightCooldown = kAttackCooldown;
	}
	if (fireDark && m_DarkCooldown <= 0.f)
	{
		FireProjectile(Alignment::Dark, mouseX, mouseY);
		m_DarkCooldown = kAttackCooldown;
	}

	UpdateMonsters(deltaTime, status);
	UpdateProjectiles(deltaTime);
	UpdateParticles(deltaTime, status);
}

void Scene::FireProjectile(Alignment alignment, float aimX, float aimY)
{
	float length = sqrtf(aimX * aimX + aimY * aimY);
	if (length < 0.001f)
	{
		return;
	}

	float worldX, worldY;
	IsoPosition(m_PlayerCol, m_PlayerRow, worldX, worldY);

	Projectile projectile;
	projectile.x = worldX;
	projectile.y = worldY;
	projectile.velX = (aimX / length) * kProjectileSpeed;
	projectile.velY = (aimY / length) * kProjectileSpeed;
	projectile.life = kProjectileLifetime;
	projectile.alignment = alignment;
	projectile.spent = false;

	m_Projectiles.push_back(projectile);
}

void Scene::SpawnMonsters()
{
	for (int i = 0; i < kMonsterCount; i++)
	{
		int col = 15 + (rand() % 90);
		int row = 15 + (rand() % 90);

		float worldX, worldY;
		IsoPosition((float)col, (float)row, worldX, worldY);

		Monster monster;
		monster.x = worldX;
		monster.y = worldY;
		monster.originX = worldX;
		monster.originY = worldY;
		monster.roamTargetX = worldX;
		monster.roamTargetY = worldY;
		monster.roamWaitTimer = RandomUnit() * 2.f;
		monster.attackCooldown = 0.f;
		monster.health = kMonsterMaxHealth;
		monster.alignment = ((rand() % 2) == 0) ? Alignment::Light : Alignment::Dark;
		monster.state = MonsterState::Roam;
		monster.alive = true;

		m_Monsters.push_back(monster);
	}
}

void Scene::SpawnDeathBurst(const Monster& monster)
{
	for (int i = 0; i < kParticleBurstCount; i++)
	{
		float angle = RandomAngle();
		float speed = kParticleBurstSpeed * (0.5f + RandomUnit() * 0.5f);

		Particle particle;
		particle.x = monster.x;
		particle.y = monster.y;
		particle.velX = cosf(angle) * speed;
		particle.velY = sinf(angle) * speed;
		particle.life = kParticleLifetime;
		particle.alignment = monster.alignment;
		particle.absorbed = false;

		m_Particles.push_back(particle);
	}
}

void Scene::UpdateMonsters(float deltaTime, PlayerStatus& status)
{
	float playerWorldX, playerWorldY;
	IsoPosition(m_PlayerCol, m_PlayerRow, playerWorldX, playerWorldY);

	for (size_t i = 0; i < m_Monsters.size(); i++)
	{
		Monster& monster = m_Monsters[i];
		if (!monster.alive)
		{
			continue;
		}

		if (monster.attackCooldown > 0.f)
		{
			monster.attackCooldown -= deltaTime;
		}

		float toPlayerX = playerWorldX - monster.x;
		float toPlayerY = playerWorldY - monster.y;
		float distToPlayer = sqrtf(toPlayerX * toPlayerX + toPlayerY * toPlayerY);

		// State transitions, with a larger give-up radius than detect radius
		// so a monster at the boundary does not flicker between roam and chase.
		if (monster.state == MonsterState::Attack)
		{
			if (distToPlayer > kMonsterAttackRadius)
			{
				monster.state = MonsterState::Chase;
			}
		}
		else if (monster.state == MonsterState::Chase)
		{
			if (distToPlayer > kMonsterGiveUpRadius)
			{
				monster.state = MonsterState::Roam;
				monster.roamTargetX = monster.originX;
				monster.roamTargetY = monster.originY;
			}
			else if (distToPlayer <= kMonsterAttackRadius)
			{
				monster.state = MonsterState::Attack;
			}
		}
		else
		{
			if (distToPlayer <= kMonsterDetectRadius)
			{
				monster.state = MonsterState::Chase;
			}
		}

		if (monster.state == MonsterState::Attack)
		{
			if (monster.attackCooldown <= 0.f && m_PlayerInvulnTimer <= 0.f)
			{
				status.Damage(kMonsterAttackDamage);
				monster.attackCooldown = kMonsterAttackCooldown;
				m_PlayerInvulnTimer = kPlayerInvulnDuration;
			}
		}
		else if (monster.state == MonsterState::Chase)
		{
			if (distToPlayer > 0.001f)
			{
				monster.x += (toPlayerX / distToPlayer) * kMonsterChaseSpeed * deltaTime;
				monster.y += (toPlayerY / distToPlayer) * kMonsterChaseSpeed * deltaTime;
			}
		}
		else
		{
			float toTargetX = monster.roamTargetX - monster.x;
			float toTargetY = monster.roamTargetY - monster.y;
			float distToTarget = sqrtf(toTargetX * toTargetX + toTargetY * toTargetY);

			if (distToTarget < 4.f)
			{
				if (monster.roamWaitTimer > 0.f)
				{
					monster.roamWaitTimer -= deltaTime;
				}
				else
				{
					float angle = RandomAngle();
					float radius = RandomUnit() * kMonsterRoamRadius;
					monster.roamTargetX = monster.originX + cosf(angle) * radius;
					monster.roamTargetY = monster.originY + sinf(angle) * radius;
					monster.roamWaitTimer = 1.5f + RandomUnit() * 2.f;
				}
			}
			else
			{
				monster.x += (toTargetX / distToTarget) * kMonsterRoamSpeed * deltaTime;
				monster.y += (toTargetY / distToTarget) * kMonsterRoamSpeed * deltaTime;
			}
		}
	}
}

void Scene::UpdateProjectiles(float deltaTime)
{
	for (size_t i = 0; i < m_Projectiles.size(); i++)
	{
		Projectile& projectile = m_Projectiles[i];
		projectile.x += projectile.velX * deltaTime;
		projectile.y += projectile.velY * deltaTime;
		projectile.life -= deltaTime;

		if (projectile.life <= 0.f)
		{
			projectile.spent = true;
			continue;
		}

		for (size_t j = 0; j < m_Monsters.size(); j++)
		{
			Monster& monster = m_Monsters[j];
			if (!monster.alive)
			{
				continue;
			}

			float dx = monster.x - projectile.x;
			float dy = monster.y - projectile.y;
			float dist = sqrtf(dx * dx + dy * dy);
			if (dist > kProjectileHitRadius)
			{
				continue;
			}

			// Both colors hurt a gray monster equally - gray is opposed to
			// color itself, not to one side of it.
			monster.health -= 1.f;
			projectile.spent = true;

			if (monster.health <= 0.f)
			{
				monster.alive = false;
				SpawnDeathBurst(monster);
			}

			break;
		}
	}

	m_Projectiles.erase(
		std::remove_if(m_Projectiles.begin(), m_Projectiles.end(), [](const Projectile& p)
		{
			return p.spent;
		}),
		m_Projectiles.end());

	m_Monsters.erase(
		std::remove_if(m_Monsters.begin(), m_Monsters.end(), [](const Monster& m)
		{
			return !m.alive;
		}),
		m_Monsters.end());
}

void Scene::UpdateParticles(float deltaTime, PlayerStatus& status)
{
	float playerWorldX, playerWorldY;
	IsoPosition(m_PlayerCol, m_PlayerRow, playerWorldX, playerWorldY);

	for (size_t i = 0; i < m_Particles.size(); i++)
	{
		Particle& particle = m_Particles[i];

		float friction = 1.f - kParticleFriction * deltaTime;
		if (friction < 0.f)
		{
			friction = 0.f;
		}
		particle.velX *= friction;
		particle.velY *= friction;

		float toPlayerX = playerWorldX - particle.x;
		float toPlayerY = playerWorldY - particle.y;
		float distToPlayer = sqrtf(toPlayerX * toPlayerX + toPlayerY * toPlayerY);

		// The player does not need to press anything to absorb a drop - just
		// get close. Anything further out just settles where it landed.
		if (distToPlayer < kParticleMagnetRadius && distToPlayer > 0.001f)
		{
			particle.velX += (toPlayerX / distToPlayer) * kParticleMagnetAccel * deltaTime;
			particle.velY += (toPlayerY / distToPlayer) * kParticleMagnetAccel * deltaTime;
		}

		particle.x += particle.velX * deltaTime;
		particle.y += particle.velY * deltaTime;
		particle.life -= deltaTime;

		if (distToPlayer < kParticleAbsorbRadius)
		{
			if (particle.alignment == Alignment::Light)
			{
				status.AddLight(1.f);
			}
			else
			{
				status.AddDark(1.f);
			}
			particle.absorbed = true;
		}
		else if (particle.life <= 0.f)
		{
			particle.absorbed = true;
		}
	}

	m_Particles.erase(
		std::remove_if(m_Particles.begin(), m_Particles.end(), [](const Particle& p)
		{
			return p.absorbed;
		}),
		m_Particles.end());
}

void Scene::Render(Renderer* renderer, const PlayerStatus& status, int windowSizeX, int windowSizeY)
{
	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;

	float camX, camY;
	IsoPosition(m_PlayerCol, m_PlayerRow, camX, camY);

	int radius = ComputeViewRadius();
	int playerCol = (int)floorf(m_PlayerCol);
	int playerRow = (int)floorf(m_PlayerRow);
	int colMin = std::max(0, playerCol - radius);
	int colMax = std::min(kMapCols - 1, playerCol + radius);
	int rowMin = std::max(0, playerRow - radius);
	int rowMax = std::min(kMapRows - 1, playerRow + radius);

	m_DrawList.clear();
	CollectTerrain(m_DrawList, camX, camY, colMin, colMax, rowMin, rowMax);
	CollectRuins(m_DrawList, camX, camY, colMin, colMax, rowMin, rowMax);
	CollectMonsters(m_DrawList, camX, camY);
	CollectProjectiles(m_DrawList, camX, camY);
	CollectParticles(m_DrawList, camX, camY);
	CollectPlayer(m_DrawList, status);

	// Terrain and the things standing on it share one ordering, so the player
	// can be hidden behind a terrace that is in front of him.
	std::stable_sort(m_DrawList.begin(), m_DrawList.end(), [](const QuadItem& a, const QuadItem& b)
	{
		return a.depth < b.depth;
	});

	for (size_t i = 0; i < m_DrawList.size(); i++)
	{
		const QuadItem& item = m_DrawList[i];
		renderer->DrawSolidQuad(item.x, item.y, item.edgeAX, item.edgeAY, item.edgeBX, item.edgeBY,
			item.r, item.g, item.b, item.a);
	}
}

void Scene::RenderMinimap(Renderer* renderer, int windowSizeX, int windowSizeY) const
{
	float left = -windowSizeX / 2.f;
	float top = windowSizeY / 2.f;

	// Sits directly under the vessel/health block in the same corner.
	float centerX = left + 95.f;
	float centerY = top - 220.f;

	renderer->DrawSolidRect(centerX, centerY, kMinimapSize + kMinimapBorder * 2.f, kMinimapSize + kMinimapBorder * 2.f,
		0.32f, 0.31f, 0.34f, 1.f);
	renderer->DrawSolidRect(centerX, centerY, kMinimapSize, kMinimapSize, 0.08f, 0.075f, 0.095f, 1.f);

	renderer->SetClip(centerX - kMinimapSize / 2.f, centerY - kMinimapSize / 2.f, kMinimapSize, kMinimapSize);

	// A plain linear col/row offset, not the iso projection - a minimap reads
	// better as a straightforward north-up map than a tilted diamond.
	float pixelsPerTile = (kMinimapSize / 2.f) / kMinimapRange;

	for (size_t i = 0; i < m_Monsters.size(); i++)
	{
		const Monster& monster = m_Monsters[i];
		if (!monster.alive)
		{
			continue;
		}

		float col, row;
		InverseIso(monster.x, monster.y, col, row);

		float dotX = centerX + (col - m_PlayerCol) * pixelsPerTile;
		float dotY = centerY - (row - m_PlayerRow) * pixelsPerTile;

		renderer->DrawSolidRect(dotX, dotY, 5.f, 5.f, 0.74f, 0.22f, 0.20f, 1.f);
	}

	renderer->ClearClip();

	// The player is always exactly at the center and always on top.
	renderer->DrawSolidRect(centerX, centerY, 8.f, 8.f, 0.30f, 0.30f, 0.34f, 1.f);
	renderer->DrawSolidRect(centerX, centerY, 6.f, 6.f, 0.94f, 0.92f, 0.88f, 1.f);
}

void Scene::CollectTerrain(std::vector<QuadItem>& out, float camX, float camY, int colMin, int colMax, int rowMin, int rowMax)
{
	for (int row = rowMin; row <= rowMax; row++)
	{
		for (int col = colMin; col <= colMax; col++)
		{
			float x, y;
			IsoPosition((float)col, (float)row, x, y);
			x -= camX;
			y -= camY;

			if (!IsOnScreen(x, y, kTileWidth, kTileHeight * 4.f))
			{
				continue;
			}

			float elevation = Elevation(col, row);
			float topY = y + elevation;
			float depth = (float)(col + row);

			// Everything is gray; only the grain and the lighting vary.
			float grain = (Hash(col, row) % 100) / 100.f * 0.04f;
			float shade = 0.17f + grain + elevation * 0.0016f;

			// The wall toward the viewer's lower right, shown where the next
			// tile that way sits lower.
			float rightDrop = elevation - Elevation(col + 1, row);
			if (rightDrop > 0.f)
			{
				QuadItem wall;
				wall.depth = depth;
				wall.x = x + kTileWidth / 4.f;
				wall.y = topY - kTileHeight / 4.f - rightDrop / 2.f;
				wall.edgeAX = kTileWidth / 2.f;
				wall.edgeAY = kTileHeight / 2.f;
				wall.edgeBX = 0.f;
				wall.edgeBY = rightDrop;
				wall.r = shade * kRightLight;
				wall.g = shade * kRightLight * 0.98f;
				wall.b = shade * kRightLight * 1.08f;
				wall.a = 1.f;
				out.push_back(wall);
			}

			float leftDrop = elevation - Elevation(col, row + 1);
			if (leftDrop > 0.f)
			{
				QuadItem wall;
				wall.depth = depth;
				wall.x = x - kTileWidth / 4.f;
				wall.y = topY - kTileHeight / 4.f - leftDrop / 2.f;
				wall.edgeAX = kTileWidth / 2.f;
				wall.edgeAY = -kTileHeight / 2.f;
				wall.edgeBX = 0.f;
				wall.edgeBY = leftDrop;
				wall.r = shade * kLeftLight;
				wall.g = shade * kLeftLight * 0.98f;
				wall.b = shade * kLeftLight * 1.08f;
				wall.a = 1.f;
				out.push_back(wall);
			}

			QuadItem top;
			top.depth = depth;
			top.x = x;
			top.y = topY;
			top.edgeAX = (kTileWidth - 1.f) / 2.f;
			top.edgeAY = (kTileHeight - 1.f) / 2.f;
			top.edgeBX = -(kTileWidth - 1.f) / 2.f;
			top.edgeBY = (kTileHeight - 1.f) / 2.f;
			top.r = shade * kTopLight;
			top.g = shade * kTopLight * 0.98f;
			top.b = shade * kTopLight * 1.06f;
			top.a = 1.f;
			out.push_back(top);
		}
	}
}

void Scene::AddBox(std::vector<QuadItem>& out, float depth, float x, float baseY,
	float footWidth, float footHeight, float boxHeight,
	float topR, float topG, float topB,
	float leftR, float leftG, float leftB,
	float rightR, float rightG, float rightB)
{
	QuadItem leftFace;
	leftFace.depth = depth;
	leftFace.x = x - footWidth / 4.f;
	leftFace.y = baseY - footHeight / 4.f + boxHeight / 2.f;
	leftFace.edgeAX = footWidth / 2.f;
	leftFace.edgeAY = -footHeight / 2.f;
	leftFace.edgeBX = 0.f;
	leftFace.edgeBY = boxHeight;
	leftFace.r = leftR;
	leftFace.g = leftG;
	leftFace.b = leftB;
	leftFace.a = 1.f;
	out.push_back(leftFace);

	QuadItem rightFace;
	rightFace.depth = depth;
	rightFace.x = x + footWidth / 4.f;
	rightFace.y = baseY - footHeight / 4.f + boxHeight / 2.f;
	rightFace.edgeAX = footWidth / 2.f;
	rightFace.edgeAY = footHeight / 2.f;
	rightFace.edgeBX = 0.f;
	rightFace.edgeBY = boxHeight;
	rightFace.r = rightR;
	rightFace.g = rightG;
	rightFace.b = rightB;
	rightFace.a = 1.f;
	out.push_back(rightFace);

	QuadItem topFace;
	topFace.depth = depth;
	topFace.x = x;
	topFace.y = baseY + boxHeight;
	topFace.edgeAX = footWidth / 2.f;
	topFace.edgeAY = footHeight / 2.f;
	topFace.edgeBX = -footWidth / 2.f;
	topFace.edgeBY = footHeight / 2.f;
	topFace.r = topR;
	topFace.g = topG;
	topFace.b = topB;
	topFace.a = 1.f;
	out.push_back(topFace);
}

void Scene::AddShadow(std::vector<QuadItem>& out, float depth, float x, float baseY, float width, float height)
{
	// A dark patch on the ground is what stops a standing thing from looking
	// like it is pasted on top of the picture.
	QuadItem shadow;
	shadow.depth = depth;
	shadow.x = x;
	shadow.y = baseY;
	shadow.edgeAX = width / 2.f;
	shadow.edgeAY = height / 2.f;
	shadow.edgeBX = -width / 2.f;
	shadow.edgeBY = height / 2.f;
	shadow.r = 0.085f;
	shadow.g = 0.08f;
	shadow.b = 0.10f;
	shadow.a = 1.f;
	out.push_back(shadow);
}

void Scene::AddDiamondMark(std::vector<QuadItem>& out, float depth, float x, float y, float width, float height,
	float r, float g, float b, float a)
{
	QuadItem mark;
	mark.depth = depth;
	mark.x = x;
	mark.y = y;
	mark.edgeAX = width / 2.f;
	mark.edgeAY = height / 2.f;
	mark.edgeBX = -width / 2.f;
	mark.edgeBY = height / 2.f;
	mark.r = r;
	mark.g = g;
	mark.b = b;
	mark.a = a;
	out.push_back(mark);
}

void Scene::AddGlow(std::vector<QuadItem>& out, float depth, float x, float y, float radius,
	Alignment alignment, float strength)
{
	if (strength <= 0.01f)
	{
		return;
	}

	float r, g, b;
	if (alignment == Alignment::Light)
	{
		// A bright ring: normal alpha blending over the gray ground reads as light.
		r = 0.95f;
		g = 0.80f;
		b = 0.42f;
	}
	else
	{
		// A near-black ring: the same blend, but it darkens what is under it -
		// a light that gives off darkness rather than brightness.
		r = 0.03f;
		g = 0.02f;
		b = 0.05f;
	}

	// Drawn largest-first, smallest-last, so the rings stack into a soft falloff
	// rather than each one fighting the last for what is on top.
	const int kRingCount = 3;
	for (int ring = kRingCount - 1; ring >= 0; ring--)
	{
		float t = (float)(ring + 1) / (float)kRingCount;
		float ringRadius = radius * t * strength;
		float ringAlpha = ((1.f - t) * 0.28f + 0.04f) * strength;

		AddDiamondMark(out, depth, x, y, ringRadius * 2.f, ringRadius, r, g, b, ringAlpha);
	}
}

void Scene::CollectRuins(std::vector<QuadItem>& out, float camX, float camY, int colMin, int colMax, int rowMin, int rowMax)
{
	for (int row = rowMin; row <= rowMax; row++)
	{
		for (int col = colMin; col <= colMax; col++)
		{
			unsigned int h = Hash(col + 7717, row + 3313);
			if (h % 100 >= 7)
			{
				continue;
			}

			float x, y;
			IsoPosition((float)col, (float)row, x, y);
			x -= camX;
			y -= camY;

			if (!IsOnScreen(x, y, kTileWidth, kTileHeight * 5.f))
			{
				continue;
			}

			float baseY = y + Elevation(col, row);
			float depth = (float)(col + row);

			int variant = (h / 100) % 3;
			float footWidth = (variant == 2) ? 40.f : 24.f;
			float footHeight = footWidth / 2.f;
			float boxHeight = (variant == 2) ? 9.f : (30.f + variant * 16.f);

			float base = 0.145f + (h % 7) * 0.004f;

			// Same depth as its tile: the sort is stable and terrain is collected
			// first, so the shadow lands on top of the ground and under the box.
			AddShadow(out, depth, x, baseY, footWidth + 8.f, (footWidth + 8.f) / 2.f);
			AddBox(out, depth, x, baseY, footWidth, footHeight, boxHeight,
				base * kTopLight, base * kTopLight, base * kTopLight * 1.12f,
				base * kLeftLight, base * kLeftLight, base * kLeftLight * 1.12f,
				base * kRightLight, base * kRightLight, base * kRightLight * 1.12f);
		}
	}
}

void Scene::CollectMonsters(std::vector<QuadItem>& out, float camX, float camY)
{
	for (size_t i = 0; i < m_Monsters.size(); i++)
	{
		const Monster& monster = m_Monsters[i];
		if (!monster.alive)
		{
			continue;
		}

		float x = monster.x - camX;
		float y = monster.y - camY;

		if (!IsOnScreen(x, y, kTileWidth, kTileHeight * 4.f))
		{
			continue;
		}

		float baseY = y + GroundHeightAt(monster.x, monster.y);
		float depth = TileDepth(monster.x, monster.y);

		// Gray the whole time it is alive - alive, it gives away nothing
		// about what it once was. Only its death does that.
		float base = 0.235f;

		AddShadow(out, depth, x, baseY, 30.f, 15.f);
		AddBox(out, depth, x, baseY, 22.f, 11.f, 24.f,
			base * kTopLight, base * kTopLight, base * kTopLight * 1.08f,
			base * kLeftLight, base * kLeftLight, base * kLeftLight * 1.08f,
			base * kRightLight, base * kRightLight, base * kRightLight * 1.08f);
	}
}

void Scene::CollectProjectiles(std::vector<QuadItem>& out, float camX, float camY)
{
	for (size_t i = 0; i < m_Projectiles.size(); i++)
	{
		const Projectile& projectile = m_Projectiles[i];

		float x = projectile.x - camX;
		float y = projectile.y - camY;

		if (!IsOnScreen(x, y, kProjectileGlowRadius, kProjectileGlowRadius))
		{
			continue;
		}

		float baseY = y + GroundHeightAt(projectile.x, projectile.y) + 14.f;
		float depth = TileDepth(projectile.x, projectile.y) + 0.3f;

		bool light = projectile.alignment == Alignment::Light;
		if (light)
		{
			AddGlow(out, depth - 0.01f, x, baseY, kProjectileGlowRadius, Alignment::Light, 1.f);
			AddDiamondMark(out, depth, x, baseY, 12.f, 8.f, 0.95f, 0.78f, 0.30f, 1.f);
		}
		else
		{
			AddGlow(out, depth - 0.01f, x, baseY, kProjectileGlowRadius, Alignment::Dark, 1.f);
			AddDiamondMark(out, depth, x, baseY, 12.f, 8.f, 0.42f, 0.30f, 0.86f, 1.f);
		}
	}
}

void Scene::CollectParticles(std::vector<QuadItem>& out, float camX, float camY)
{
	for (size_t i = 0; i < m_Particles.size(); i++)
	{
		const Particle& particle = m_Particles[i];

		float x = particle.x - camX;
		float y = particle.y - camY;

		if (!IsOnScreen(x, y, 16.f, 16.f))
		{
			continue;
		}

		float baseY = y + GroundHeightAt(particle.x, particle.y) + 10.f;
		float depth = TileDepth(particle.x, particle.y) + 0.3f;

		bool light = particle.alignment == Alignment::Light;
		if (light)
		{
			AddDiamondMark(out, depth, x, baseY, 7.f, 5.f, 0.92f, 0.76f, 0.32f, 1.f);
		}
		else
		{
			AddDiamondMark(out, depth, x, baseY, 7.f, 5.f, 0.40f, 0.28f, 0.82f, 1.f);
		}
	}
}

void Scene::CollectPlayer(std::vector<QuadItem>& out, const PlayerStatus& status)
{
	// The camera is centered on the player, so he stands at the screen origin,
	// lifted by whatever terrace he is on. Both height and depth are locked to
	// the single tile nearest him (not his exact fractional position) - flooring
	// this instead would hand him the wrong tile's height for half of every
	// step, and a plain continuous depth lets a neighboring tile leapfrog his
	// depth and paint over him before he has visually reached it.
	int tileCol = (int)(m_PlayerCol + 0.5f);
	int tileRow = (int)(m_PlayerRow + 0.5f);
	const float x = 0.f;
	float baseY = Elevation(tileCol, tileRow);
	float depth = (float)(tileCol + tileRow) + 0.6f;

	// The two halves of the vessel spill onto the ground as their own light -
	// how full each side is, made visible before the HUD even has to say so.
	AddGlow(out, depth - 0.02f, x, baseY, kPlayerGlowRadius, Alignment::Light, status.light / status.capacity);
	AddGlow(out, depth - 0.02f, x, baseY, kPlayerGlowRadius, Alignment::Dark, status.dark / status.capacity);

	AddShadow(out, depth - 0.01f, x, baseY, 26.f, 13.f);

	float legsHeight = 13.f;
	float torsoHeight = 20.f;
	float headHeight = 12.f;

	// Legs
	float legsBase = 0.21f;
	AddBox(out, depth, x, baseY, 20.f, 10.f, legsHeight,
		legsBase * kTopLight, legsBase * kTopLight, legsBase * kTopLight * 1.15f,
		legsBase * kLeftLight, legsBase * kLeftLight, legsBase * kLeftLight * 1.15f,
		legsBase * kRightLight, legsBase * kRightLight, legsBase * kRightLight * 1.15f);

	// The torso carries both powers - one on each face. Tilting toward a side
	// brightens that face and drains the other toward gray.
	float lightStrength = Clamp(0.5f + status.tilt * 0.5f, 0.f, 1.f);
	float darkStrength = 1.f - lightStrength;

	float lightR = 0.30f + 0.62f * lightStrength;
	float lightG = 0.26f + 0.44f * lightStrength;
	float lightB = 0.24f + 0.10f * lightStrength;

	float darkR = 0.26f + 0.20f * darkStrength;
	float darkG = 0.24f + 0.10f * darkStrength;
	float darkB = 0.28f + 0.42f * darkStrength;

	AddBox(out, depth + 0.01f, x, baseY + legsHeight, 24.f, 12.f, torsoHeight,
		0.60f, 0.58f, 0.60f,
		darkR, darkG, darkB,
		lightR, lightG, lightB);

	// Head
	float headBase = 0.78f;
	AddBox(out, depth + 0.02f, x, baseY + legsHeight + torsoHeight, 15.f, 8.f, headHeight,
		headBase * kTopLight, headBase * kTopLight * 0.97f, headBase * kTopLight * 0.92f,
		headBase * kLeftLight, headBase * kLeftLight * 0.97f, headBase * kLeftLight * 0.92f,
		headBase * kRightLight, headBase * kRightLight * 0.97f, headBase * kRightLight * 0.92f);
}
