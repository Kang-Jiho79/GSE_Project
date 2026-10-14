#pragma once

#include "SceneGraph.h"
#include "DrawList.h"
#include "Impact.h"
#include "Regions.h"
#include "Settlement.h"
#include "Prologue.h"
#include "StoryEvents.h"
#include "Cutscene.h"

class Renderer;
class TextRenderer;
class PlayerActor;
class KingActor;
class ArenaActor;
class NpcActor;
struct PlayerStatus;
struct Progress;

// The world he is in. Builds its actor tree - the one world every place on
// the map lies in, or a place off it with a map of its own - hands it the
// frame, and turns what it collected into draw calls. All the world's
// behaviour lives in the actors; this owns only the layout, which place he
// is in, setting out and clearing away each place's monsters as he comes and
// goes, the camera (and the cutscenes that take it over), travel, the fades
// that cover it and death, and the asking for a king's duel.
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

	// The place he is in.
	int CurrentRegion() const;
	// Its air - mixing into the next place's as he nears it.
	Regions::Atmosphere CurrentAtmosphere() const;
	// One flag per region: whether he has been there.
	const bool* VisitedRegions() const;
	// Where he stands, in tiles.
	void PlayerTile(float& col, float& row) const;
	// Where he is on the world map: his place and tile - or, inside a
	// castle, its capital and the castle's door.
	void MapLocation(int& region, float& col, float& row) const;

	// Through the castle door he is at: into the throne room, or back out to
	// the capital, before its door.
	void UseDoor();
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
	// The last fight is under way or over, or a cutscene is playing: nothing
	// should be saved.
	bool HoldsSave() const;

	// Plays a cutscene anchored at this region's waystone, or at a given
	// world point. Ignored during a fade, a duel or another cutscene. While
	// it plays he has no say, the HUD is hidden and nothing can hurt him.
	void PlayCutscene(int id);
	void PlayCutscene(int id, float anchorX, float anchorY);
	bool IsInCutscene() const;
	// R with the gauge full: the gauge is spent and his form's cut-in plays;
	// the special goes off as it ends, toward where he aimed (screen offset
	// from him, in window pixels). Works in a duel too. Returns whether it
	// began.
	bool BeginLimit(PlayerStatus& status, float aimX, float aimY);
	// How far the world is drained of colour right now, 0..1.
	float Desaturation() const;

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
		Door,     // at a castle's door, outside or in
		Board,    // at a notice board with requests on it
	};
	// Nothing at all while a duel is under way.
	Interaction AvailableInteraction() const;

	// Rests at this region's altar: his wounds close, if he falls he will
	// rise here, wherever he is when it happens - and the world's monsters
	// all come back.
	void UseTemple(PlayerStatus& status);

	// Asks the king before him for a duel: he is put in the king's form and
	// held there, made whole, and it begins. Falling in it does not kill him.
	void BeginDuel(PlayerStatus& status);

	// The conversation the person beside him would start, or -1.
	int TalkScript() const;

	// A place's errands set out afresh - a request just taken there may have
	// someone or something to put in it. Nothing if it is not set out now.
	void RefreshRegion(int region);

private:
	enum class Fade
	{
		None,
		Death,  // out, he is moved to where he last rested and revived, in
		Travel, // out, the next region is built, in
	};

	// Puts him at a place's waystone: in the world, a step across it - built
	// first if it is not - or, for a place off the map, its own map built
	// round him.
	void EnterRegion(int region);
	void BuildWorld();
	void BuildIsolated(int region);
	void AddPlayer(float x, float y);
	void BuildSettlement(int region, Actor* environment);
	// He has stepped into another place: what the story has there, and its
	// name shown.
	void SetCurrent(int region);
	// Setting out and clearing away the places' monsters and bosses as he
	// comes and goes; all of them at once, after a rest.
	void UpdatePlaces();
	void Activate(int region);
	void Deactivate(int region);
	void RefillAll();
	void SpawnMonsters(int region);
	// The hidden village shut in while the gray comes for it, and a board
	// shut while its boss is awake and he is on it.
	void UpdateSeals();
	// A boss of the god's pieces awake in a place, if any: its place.
	int AwakeBossRegion();

	void WaystoneTile(float& col, float& row) const;
	void ArrivalPoint(int region, float& x, float& y) const;
	// Where he gets up in a place: beside its altar if that is where he last
	// rested, otherwise beside its waystone.
	void RestPoint(int region, float& x, float& y) const;
	bool IsNear(float col, float row, float reach) const;

	ActorContext MakeContext(float deltaTime, PlayerStatus& status);
	// What a cutscene's shot asks for as it begins.
	void PlayCue(int cue, PlayerStatus& status);
	// Whether he has put the collusion to this region's king.
	bool AskedKing() const;
	// The camera onto him at no zoom, or wherever a playing cutscene has it.
	void SettleCamera();
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
	// Whether he had already asked this king, as of the last frame: the
	// moment it changes, the god steps in.
	bool m_AskedKing = false;
	// The world has gone white over the king: on to Bellum.
	bool m_Intervening = false;

	// The castle door he stands at leads here, or -1; and whether the travel
	// under way is through such a door, to be set down before it.
	int m_DoorTarget = -1;
	bool m_DoorTravel = false;
	// Standing at this place's notice board.
	bool m_AtBoard = false;

	Prologue m_Prologue;
	StoryEvents m_Events;
	int m_Conversation = -1;

	// The one world built, rather than a place off the map; which places'
	// monsters are set out; each place's settlement (empty where there is
	// none) and walled board. The boards are owned by the graph and last as
	// long as the world does.
	bool m_InWorld = false;
	bool m_Active[Regions::Count] = {};
	Settlement::Layout m_Settlements[Regions::Count];
	ArenaActor* m_Arenas[Regions::Count] = {};
	bool m_LiminaSealed = false;
	float m_LiminaSeal[4][4] = {};

	Cutscene m_Cutscene;
	// Where the camera looks and how close: on him at no zoom, unless a
	// cutscene has it. Settled each update, before anything is drawn.
	float m_CamX = 0.f;
	float m_CamY = 0.f;
	float m_Zoom = 1.f;

	// The capital's king, if this is a capital. Owned by the graph; a king is
	// never destroyed, so this stays good until the region is left.
	KingActor* m_King = nullptr;

	float m_BannerTimer = 0.f; // the region's name, shown as he arrives

	Fade m_Fade = Fade::None;
	float m_FadeElapsed = 0.f;
	int m_TravelTarget = 0;
	// Whether it was the gray that took him, for what is written over it.
	bool m_DeathByGray = false;
};
