#pragma once

#include "SceneGraph.h"
#include "DrawList.h"
#include "Impact.h"
#include "Regions.h"
#include "Settlement.h"

class Renderer;
class TextRenderer;
class PlayerActor;
struct PlayerStatus;

// The region he is in. Builds its actor tree from the region's entry in the
// table, hands it the frame, and turns what it collected into draw calls.
// All the world's behaviour lives in the actors; this owns only the layout,
// the camera, travel between regions, and the fades that cover it and death.
class Scene
{
public:
	Scene();

	void SetViewport(int windowSizeX, int windowSizeY);

	void Update(const InputState& input, float deltaTime, PlayerStatus& status);
	void Render(Renderer* renderer, PlayerStatus& status);
	void RenderMinimap(Renderer* renderer);
	// Words drawn with the HUD, over the world: the region's name as he
	// arrives, what to press at a waystone, the line over the dark when he
	// falls.
	void RenderText(TextRenderer* text) const;

	// Sprays, hit stops and screen shake, together.
	void SetEffectsEnabled(bool enabled);
	bool AreEffectsEnabled() const;

	int CurrentRegion() const;
	const Regions::Atmosphere& CurrentAtmosphere() const;
	// One flag per region: whether he has been there.
	const bool* VisitedRegions() const;
	// Fades out, builds the region, and fades back in with him at its waystone.
	void BeginTravel(int region);

	// What F would do where he stands - on his feet, with no fade under way.
	enum class Interaction
	{
		None,
		WorldMap, // at the waystone
		Temple,   // at an altar
	};
	Interaction AvailableInteraction() const;

	// Rests at this region's altar: his wounds close, and if he falls he will
	// rise here, wherever he is when it happens.
	void UseTemple(PlayerStatus& status);

private:
	enum class Fade
	{
		None,
		Death,  // out, he is moved to where he last rested and revived, in
		Travel, // out, the next region is built, in
	};

	void EnterRegion(int region);
	void SpawnMonsters(const Regions::Info& info);
	void BuildSettlement(const Regions::Info& info, Actor* environment);
	void WaystoneTile(float& col, float& row) const;
	void ArrivalPoint(float& x, float& y) const;
	// Where he rises after falling: beside the altar he last rested at if he
	// is in its region, otherwise beside the waystone.
	void RespawnPoint(float& x, float& y) const;
	bool IsNear(float col, float row, float reach) const;

	ActorContext MakeContext(float deltaTime, PlayerStatus& status);
	void RecordPopulation();

	void UpdateFade(float deltaTime, PlayerStatus& status);
	void FadeTimes(float& fadeOut, float& hold, float& fadeIn) const;
	// How dark the fade is right now, 0..1.
	float FadeAlpha() const;
	void DrawFade(Renderer* renderer) const;
	// The world washing toward gray as his balance goes.
	void DrawErosionTint(Renderer* renderer, const PlayerStatus& status) const;

	SceneGraph m_Graph;
	DrawList m_DrawList;
	Impact m_Impact;

	// Owned by the graph; kept only because the camera and the monsters both
	// need to know where he is.
	PlayerActor* m_Player = nullptr;

	int m_WindowSizeX = 0;
	int m_WindowSizeY = 0;

	int m_Region = Regions::Limina;
	bool m_Visited[Regions::Count] = {};
	bool m_AtWaystone = false;
	bool m_AtAltar = false;

	Settlement::Layout m_Settlement;

	// The last temple he rested at; until he finds one, the hidden village.
	int m_RespawnRegion = Regions::Limina;
	bool m_RespawnAtAltar = false;
	float m_BannerTimer = 0.f; // the region's name, shown as he arrives

	Fade m_Fade = Fade::None;
	float m_FadeElapsed = 0.f;
	int m_TravelTarget = 0;
	// Whether it was the gray that took him, for what is written over it.
	bool m_DeathByGray = false;
};
