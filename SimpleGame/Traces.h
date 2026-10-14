#pragma once

// What is left of light and dark on the gray ground, tile by tile: gold
// cracks, guttering lamps, statues with only their halos left, scraps of gold
// banner and motes of light rising - or violet cracks, ink-black thorns,
// violet embers, dark crystals and shadow sinking - and on the battlefields,
// the broken weapons of both and cracks flickering between the two colours.
//
// Nothing is stored. Like Props, what lies where is worked out from the tile
// and the map's seed. How thickly traces lie is each place's own, and so is
// how far it leans light - the same lean its monsters have - but within a
// place they gather in patches, gold here and violet there, and the gray of
// each side keeps to its own patches (LightLean). Reading the ground is
// reading which power a fight will give.
namespace Traces
{
	enum class Kind
	{
		None,

		LightCrack,
		Lamp,
		HaloStatue,
		Banner,
		LightMotes,

		DarkCrack,
		Thorns,
		Embers,
		Crystal,
		ShadowMotes,

		Wreck,        // weapons and banners of both, broken
		FlickerCrack, // a crack flickering gold and violet by turns
	};

	// Nowhere any traces, until each place is given its own.
	void Clear();
	// A place's traces: how thickly they lie (1 at the thickest), how far
	// they lean light, 0..1, and what share of them are of both sides at once.
	void SetPlace(int zone, float density, float lightShare, float mixedShare);

	// How far the ground here leans light, 0..1: the place's own lean, swung
	// either way in broad patches - widely where the sides are even, barely
	// where one holds the land - and pushed toward one side or the other.
	float LightLean(int col, int row);

	// What lies on a tile; variant varies its look.
	Kind At(int col, int row, int& variant);
	// Whether it gives off light of its own.
	bool Glows(Kind kind);
}
