#pragma once
#include "PartitionStats.hpp"
#include "RegionStats.hpp"
#include "RegionStatsMap.hpp"

#include <strmath.hpp>

inline std::ostream& operator<<(std::ostream& os, const cv::Point& p)
{
	return os << p.x << "," << p.y;
}

inline std::ostream& operator<<(std::ostream& os, const RegionStats& stats)
{
	os << '[';
	const auto& filtered{ stats.filter_region_area() };
	for (auto it{ filtered.begin() }, end{ filtered.end() }; it != end; ++it) {
		os << '(' << *it << ')';
		if (std::distance(it, end) > 1ull)
			os << ", ";
	}
	return os << ']';
}

inline std::ostream& operator<<(std::ostream& os, const RegionStatsMap& regions)
{
	for (const auto& [region, stats] : regions)
		os << region.Name() << " = " << stats << '\n';
	return os;
}
