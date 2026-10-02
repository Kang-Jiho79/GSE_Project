#pragma once

class SceneGraph;

// Where an attack becomes damage. Every damage number is worked out here, so
// changing how hard something hits never means hunting through the actors.
namespace Combat
{
	float BoltDamage();
	float SlashDamage(int comboStep);

	// Damages every monster inside a wedge on the ground. The wedge is measured
	// in tiles, not screen pixels, so it covers the same ground whichever way
	// it points. dirCol/dirRow must be a unit vector.
	void HitArc(SceneGraph& graph, float originX, float originY, float dirCol, float dirRow,
		float rangeTiles, float arcDegrees, float damage);
}
