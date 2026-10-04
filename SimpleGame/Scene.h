#pragma once

#include "SceneGraph.h"
#include "DrawList.h"
#include "Impact.h"
#include "Regions.h"
#include "Settlement.h"
#include "Prologue.h"
#include "StoryEvents.h"

class Renderer;
class TextRenderer;
class PlayerActor;
class KingActor;
class NpcActor;
struct PlayerStatus;
struct Progress;

// The region he is in. Builds its actor tree from the region's entry in the
// table, hands it the frame, and turns what it collected into draw calls.
// All the world's behaviour lives in the actors; this owns only the layout,
// the camera, travel between regions, the fades that cover it and death, and
// the asking for a king's duel.
class Scene
{
public:
	// Builds nothing yet; call NewGame or Continue before the first frame.
	explicit Scene(Progress& progress);

	void SetViewport(int windowSizeX, int windowSizeY);

	void Update(const InputState& input, float deltaTime, PlayerStatus& status);
	void Render(Renderer* renderer, PlayerStatus& status);
	void RenderMinimap(Renderer* renderer);
	// Words drawn with the HUD, over the world: the region's name as he
	// arrives, what to press at a waystone, the line over the dark when he
	// falls.
	void RenderText(TextRenderer* text) const;
	// The name and health of a boss he is fighting, low in the middle.
	void RenderBossBar(Renderer* renderer, TextRenderer* text);

	// Sprays, hit stops and screen shake, together.
	void SetEffectsEnabled(bool enabled);
	bool AreEffectsEnabled() const;

	int CurrentRegion() const;
	const Regions::Atmosphere& CurrentAtmosphere() const;
	// One flag per region: whether he has been there.
	const bool* VisitedRegions() const;
	// From the start: the hidden village on its last day. Progress and
	// status must already be fresh.
	void NewGame();
	// Back where a saved journey left off: at the region's waystone, or its
	// altar if he last rested there. Progress and status must already be
	// loaded.
	void Continue(int region);

	// A conversation the story wants started now, or -1. Taken once.
	int TakeConversation();
	// An ending the story has reached (a Dialogue::EndingId), or -1. Taken once.
	int TakeEnding();
	// The last fight is under way or over: nothing should be saved.
	bool HoldsSave() const;

	// Fades out, builds the region, and fades back in with him at its waystone.
	// Ignored during a fade or a duel.
	void BeginTravel(int region);

	// What F would do where he stands - on his feet, with no fade under way.
	enum class Interaction
	{
		None,
		WorldMap, // at the waystone
		Temple,   // at an altar
		Duel,     // before a king who will still fight him
		Talk,     // beside someone who will talk to him
	};
	// Nothing at all while a duel is under way.
	Interaction AvailableInteraction() const;

	// Rests at this region's altar: his wounds close, and if he falls he will
	// rise here, wherever he is when it happens.
	void UseTemple(PlayerStatus& status);

	// Asks the king before him for a duel: he is put in the king's form and
	// held there, made whole, and it begins. Falling in it does not kill him.
	void BeginDuel(PlayerStatus& status);

	// The conversation the person beside him would start, or -1.
	int TalkScript() const;

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

	// Where he has been, where he rises, what he has beaten - kept outside
	// so it outlives the regions and can be saved.
	Progress& m_Progress;

	int m_Region = Regions::Limina;
	bool m_AtWaystone = false;
	bool m_AtAltar = false;
	bool m_AtKing = false;
	// Whoever he could talk to from where he stands. Owned by the graph, and
	// found afresh every frame, after anything that could take them away.
	NpcActor* m_Listener = nullptr;

	// A king that has acknowledged him, near enough to speak to.
	bool m_AtKingToTalk = false;

	Prologue m_Prologue;
	StoryEvents m_Events;
	int m_Conversation = -1;

	// The capital's king, if this is a capital. Owned by the graph; a king is
	// never destroyed, so this stays good until the region is left.
	KingActor* m_King = nullptr;

	Settlement::Layout m_Settlement;
	float m_BannerTimer = 0.f; // the region's name, shown as he arrives

	Fade m_Fade = Fade::None;
	float m_FadeElapsed = 0.f;
	int m_TravelTarget = 0;
	// Whether it was the gray that took him, for what is written over it.
	bool m_DeathByGray = false;
};
