#pragma once
#include "Region.hpp"

#include <opencv2/opencv.hpp>

#include <ostream>
#include <map>
#include <vector>


/**
 * @class	ColorMap
 * @brief	A map where the keys are `RGB` colors, and the values are `Region` types.
 */
class ColorMap : public std::map<RGB, Region> {
	using base = std::map<RGB, Region>;
public:
	using base::base;

	/**
	 * @brief				ColorMap constructor that uses a vector of regions to build itself.
	 * @param regionVec		Vector of regions to use for building the map.
	 */
	ColorMap(RegionVec const& regionVec) : base()
	{
		for (const auto& region : regionVec)
			this->insert_or_assign(region.color, region);
	}
};

///// @brief	A vector of pairs where the first element is a `cv::Point` and the second is a vector of `Region` types. This is used as an intermediary type between the raw input image, and the output file.
using HoldMap = std::vector<std::pair<cv::Point, std::vector<Region>>>;

// file writing operators:
inline std::ostream& operator<<(std::ostream& os, const HoldMap& holdmap)
{
	for (const auto& [pos, regions] : holdmap)
		os << '(' << pos.x << ',' << pos.y << ") = " << regions << '\n';
	return os;
}
