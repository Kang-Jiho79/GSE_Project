#pragma once

class SceneGraph;
struct PlayerStatus;
struct ActorContext;

// Where an attack becomes damage. Every damage number is worked out here, so
// changing how hard something hits never means hunting through the actors.
//
// Each is its base value scaled by everything he has absorbed of that power:
// light attacks by his lifetime light, dark attacks by his lifetime dark.
// Feeding only one side leaves the other form's attacks behind.
namespace Combat
{
	float BoltDamage(const PlayerStatus& status);
	float BeamDamage(const PlayerStatus& status);
	float StrikeDamage(const PlayerStatus& status);

	float SlashDamage(const PlayerStatus& status, int comboStep);
	float DashDamage(const PlayerStatus& status);
	float EruptDamage(const PlayerStatus& status);
	// The dodges, once they strike.
	float BlinkBurstDamage(const PlayerStatus& status);
	float ShadowDashDamage(const PlayerStatus& status);
	float DarkBurstDamage(const PlayerStatus& status);

	// The gray's, scaled by both lifetime totals together (their average),
	// since it is both powers at once.
	float AshWaveDamage(const PlayerStatus& status);
	float UnravelDamage(const PlayerStatus& status);
	float EraseDamage(const PlayerStatus& status);

	// Health returned for melee damage that landed. scale raises the usual
	// share for moves that feed it harder.
	float Lifesteal(const PlayerStatus& status, float damageLanded, float scale = 1.f);

	// Damages every enemy inside a wedge on the ground. The wedge is measured
	// in tiles, not screen pixels, so it covers the same ground whichever way
	// it points. dirCol/dirRow must be a unit vector. Returns the damage that
	// landed across everything hit.
	float HitArc(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
		float rangeTiles, float arcDegrees, float damage);

	// Damages every monster within widthTiles of a straight line on the ground,
	// from the origin out to lengthTiles along a unit direction. Returns the
	// damage that landed. blockable: a light blow that a shield turned toward
	// the origin can catch. Everything it touches is pierced first.
	float HitLine(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
		float lengthTiles, float widthTiles, float damage, bool blockable);

	// A monster's blow at the player: lands if the player stands inside the
	// wedge or circle on the ground. Returns whether it hit - a roll or the
	// moment after another hit can still turn it aside.
	bool StrikePlayerArc(const ActorContext& ctx, float originX, float originY, float dirCol, float dirRow,
		float rangeTiles, float arcDegrees, float damage);
	bool StrikePlayerRadius(const ActorContext& ctx, float centerX, float centerY,
		float radiusTiles, float damage);
	// The same along a band on the ground: within widthTiles of the line from
	// the origin out to lengthTiles along a unit direction.
	bool StrikePlayerLine(const ActorContext& ctx, float originX, float originY, float dirCol, float dirRow,
		float lengthTiles, float widthTiles, float damage);

	// Damages, then shoves, every monster within a circle on the ground. A
	// positive shove pushes away from the centre; a negative one pulls toward
	// it, stopping short of it. Returns the damage that landed.
	float HitRadius(SceneGraph& graph, float centerX, float centerY,
		float radiusTiles, float damage, float shoveTiles);
}
