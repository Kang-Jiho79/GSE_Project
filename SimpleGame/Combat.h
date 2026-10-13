#pragma once

class SceneGraph;
class EnemyActor;
struct PlayerStatus;
struct ActorContext;

// Where an attack becomes damage. Every damage number is worked out here, so
// changing how hard something hits never means hunting through the actors.
//
// Each is its base value scaled by everything he has absorbed of that power:
// light attacks by his lifetime light, dark attacks by his lifetime dark.
// Feeding only one side leaves the other form's attacks behind. The gray's
// alone are fixed: taking it up, he gave the other two away.
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

	// The gray's: the scythe's cuts and the last cut's trail bursting again,
	// the dash cut and the blast where it left, the counter (reached: it
	// found someone to cut, rather than lashing out all round), E and Q.
	float ScytheDamage(int comboStep);
	float ScytheEchoDamage();
	float DashCutDamage();
	float DashBlastDamage();
	float CounterDamage(bool reached);
	float UnravelDamage();
	float EraseDamage();

	// The specials (R).
	float PillarDamage(const PlayerStatus& status);
	float FinalPillarDamage(const PlayerStatus& status);
	float GrabDamage(const PlayerStatus& status);
	float GrabBlastDamage(const PlayerStatus& status);
	float ReapDamage();

	// Every blow of a special is struck between Begin and End with its own
	// number: a boss counts what one special has taken from it against its
	// cap, and none of it fills the gauge again. 0 outside.
	int NewLimitSerial();
	void BeginLimitBlow(int serial);
	void EndLimitBlow();
	int LimitBlow();

	// Damage he has landed since this was last asked, for the gauge. The
	// three Hit functions note it themselves; anything striking an enemy
	// directly notes its own.
	void NoteLanded(float landed);
	float TakeLanded();

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

	// The nearest enemy within reach of a point (tiles, counted to the edge
	// of its body) - only inside the wedge round a unit direction, unless the
	// spread is a full turn. nullptr for none.
	EnemyActor* NearestEnemy(SceneGraph& graph, float x, float y, float reachTiles,
		float dirCol, float dirRow, float spreadDegrees);

	// Damages, then shoves, every monster within a circle on the ground. A
	// positive shove pushes away from the centre; a negative one pulls toward
	// it, stopping short of it. Returns the damage that landed.
	float HitRadius(SceneGraph& graph, float centerX, float centerY,
		float radiusTiles, float damage, float shoveTiles);
}
