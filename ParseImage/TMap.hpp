#pragma once
#include "Region.hpp"

#include <opencv2/opencv.hpp>

#include <ostream>
#include <map>
#include <vector>

/// @brief	A map where the keys are `RGB` colors, and the values are `Region` enum types.
using ColorMap = std::map<RGB, Region>;
///// @brief	A vector of pairs where the first element is a `cv::Point` and the second is a vector of `Region` enum types. This is used as an intermediary type between the raw input image, and the output file.
using HoldMap = std::vector<std::pair<cv::Point, std::vector<Region>>>;

// file writing operators:
inline std::ostream& operator<<(std::ostream& os, const HoldMap& holdmap)
{
	for (const auto& [pos, regions] : holdmap)
		os << '(' << pos.x << ',' << pos.y << ") = " << regions << '\n';
	return os;
}
