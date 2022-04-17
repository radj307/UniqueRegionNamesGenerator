#pragma once
/**
 * @enum	Region
 * @brief	Enumerator that defines the recognized regions of Skyrim on the map. This is used by `ColorMap` types to map regions to colors for the purpose of image parsing.
 */
enum class Region : char {
	None,
	Solitude,			// Haafingar
	Morthal_Hold,		// Hjaalmarch
	Markarth,			// Reach
	Whiterun,			// Whiterun
	Falkreath,			// Falkreath
	Dawnstar,			// Pale
	Winterhold_Hold,	// Winterhold
	Windhelm,			// Eastmarch
	Riften,				// Rift

	Riverwood,			// Riverwood
	Winterhold,			// Winterhold City
	Helgen,				// Helgen
	Rorikstead,			// Rorikstead
	Morthal,			// Morthal City
	GhostSea,			// Ghost Sea
};
