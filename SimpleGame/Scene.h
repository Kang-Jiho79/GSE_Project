#pragma once

#include "Renderer.h"
#include "PlayerStatus.h"
#include <vector>

// Quarter-view world with real relief: the terrain is terraced, so every step
// down shows a wall face, and everything standing on it is drawn as a box with
// three lit faces rather than a flat card.
//
// Level 1's population: gray monsters that roam until the player is close,
// then chase and attack; light/dark projectiles the player throws at the
// mouse cursor; and the light/dark particles a dead monster scatters, which
// the player's own presence quietly pulls in and banks into the vessel.
class Scene
{
public:
	Scene();

	// screenRight/screenUp are each -1..1 and are screen directions, not grid
	// axes, so one key moves straight along a screen axis and two adjacent keys
	// blend into a diagonal at the same speed. mouseX/mouseY are pixels relative
	// to the window center (the player's own screen position, always the origin).
	void Update(float screenRight, float screenUp, bool fireLight, bool fireDark,
		float mouseX, float mouseY, float deltaTime, PlayerStatus& status);
	void Render(Renderer* renderer, const PlayerStatus& status, int windowSizeX, int windowSizeY);
	// A plain top-down panel (not the tilted iso view) - player fixed at its
	// center, nearby monsters as dots. Placed just under the vessel/health HUD.
	void RenderMinimap(Renderer* renderer, int windowSizeX, int windowSizeY) const;

private:
	enum class Alignment
	{
		Light,
		Dark,
	};

	enum class MonsterState
	{
		Roam,
		Chase,
		Attack,
	};

	struct Monster
	{
		float x, y;
		float originX, originY;
		float roamTargetX, roamTargetY;
		float roamWaitTimer;
		float attackCooldown;
		float health;
		Alignment alignment;
		MonsterState state;
		bool alive;
	};

	struct Projectile
	{
		float x, y;
		float velX, velY;
		float life;
		Alignment alignment;
		bool spent;
	};

	struct Particle
	{
		float x, y;
		float velX, velY;
		float life;
		Alignment alignment;
		bool absorbed;
	};

	struct QuadItem
	{
		float depth;
		float x, y;
		float edgeAX, edgeAY;
		float edgeBX, edgeBY;
		float r, g, b, a;
	};

	void IsoPosition(float col, float row, float& outX, float& outY) const;
	void InverseIso(float worldX, float worldY, float& outCol, float& outRow) const;
	// Which single tile a continuous world position sits over - the nearest grid
	// cell, since each tile's diamond owns the +/-0.5 span around its own integer
	// (col, row). Used to keep an entity's height and depth locked to a whole
	// tile at a time instead of drifting continuously between tiles.
	void WorldToTile(float worldX, float worldY, int& outCol, int& outRow) const;
	float TileDepth(float worldX, float worldY) const;
	bool IsOnScreen(float x, float y, float halfExtentX, float halfExtentY) const;
	int ComputeViewRadius() const;
	float GroundHeightAt(float worldX, float worldY) const;

	void CollectTerrain(std::vector<QuadItem>& out, float camX, float camY, int colMin, int colMax, int rowMin, int rowMax);
	void CollectRuins(std::vector<QuadItem>& out, float camX, float camY, int colMin, int colMax, int rowMin, int rowMax);
	void CollectPlayer(std::vector<QuadItem>& out, const PlayerStatus& status);
	void CollectMonsters(std::vector<QuadItem>& out, float camX, float camY);
	void CollectProjectiles(std::vector<QuadItem>& out, float camX, float camY);
	void CollectParticles(std::vector<QuadItem>& out, float camX, float camY);

	// One box: the two side faces first, then the top, so the top covers the seams.
	void AddBox(std::vector<QuadItem>& out, float depth, float x, float baseY,
		float footWidth, float footHeight, float boxHeight,
		float topR, float topG, float topB,
		float leftR, float leftG, float leftB,
		float rightR, float rightG, float rightB);
	void AddShadow(std::vector<QuadItem>& out, float depth, float x, float baseY, float width, float height);
	void AddDiamondMark(std::vector<QuadItem>& out, float depth, float x, float y, float width, float height,
		float r, float g, float b, float a);
	// Several soft, translucent rings stacked large-to-small. With Light this reads
	// as a glow; with Dark, blending a near-black ring over the ground instead
	// literally darkens it - a light that gives off darkness rather than brightness.
	// strength (0..1) scales both size and opacity, e.g. with how full that side
	// of the vessel is.
	void AddGlow(std::vector<QuadItem>& out, float depth, float x, float y, float radius,
		Alignment alignment, float strength);

	void SpawnMonsters();
	void SpawnDeathBurst(const Monster& monster);
	void FireProjectile(Alignment alignment, float aimX, float aimY);

	void UpdateMonsters(float deltaTime, PlayerStatus& status);
	void UpdateProjectiles(float deltaTime);
	void UpdateParticles(float deltaTime, PlayerStatus& status);

	std::vector<Monster> m_Monsters;
	std::vector<Projectile> m_Projectiles;
	std::vector<Particle> m_Particles;

	// Kept across frames so the per-frame collect does not reallocate.
	std::vector<QuadItem> m_DrawList;

	float m_LightCooldown = 0.f;
	float m_DarkCooldown = 0.f;
	float m_PlayerInvulnTimer = 0.f;

	float m_PlayerCol = 60.f;
	float m_PlayerRow = 60.f;
	int m_WindowSizeX = 0;
	int m_WindowSizeY = 0;
};
