#pragma once

class SceneGraph;

// Where an attack becomes damage. Every damage number is worked out here, so
// changing how hard something hits never means hunting through the actors.
namespace Combat
{
	float BoltDamage();
	float SlashDamage(int comboStep);

	float DarkBurstDamage();
	float BeamDamage();
	float StrikeDamage();
	float DashDamage();
	float SpinDamage();

	// Health returned for melee damage that landed. scale raises the usual
	// share for moves that feed it harder.
	float Lifesteal(float damageLanded, float scale = 1.f);

	// Damages every monster inside a wedge on the ground. The wedge is measured
	// in tiles, not screen pixels, so it covers the same ground whichever way
	// it points. dirCol/dirRow must be a unit vector. Returns the damage that
	// landed across everything hit.
	float HitArc(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
		float rangeTiles, float arcDegrees, float damage);

	// Damages every monster within widthTiles of a straight line on the ground,
	// from the origin out to lengthTiles along a unit direction. Returns the
	// damage that landed.
	float HitLine(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
		float lengthTiles, float widthTiles, float damage);

	// Damages, then shoves, every monster within a circle on the ground. A
	// positive shove pushes away from the centre; a negative one pulls toward
	// it, stopping short of it. Returns the damage that landed.
	float HitRadius(SceneGraph& graph, float centerX, float centerY,
		float radiusTiles, float damage, float shoveTiles);
}
