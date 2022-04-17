#pragma once
#include "types.hpp"
#include "PartitionStats.hpp"

#include <strmath.hpp>

inline std::ostream& operator<<(std::ostream& os, const std::vector<Region>& regionList)
{
	os << "[ ";
	for (auto it{ regionList.begin() }, endit{ regionList.end() }; it != endit; ++it) {
		os << '"' << *it << '"';
		if (std::distance(it, regionList.end()) > 1ull)
			os << ", ";
	}
	return os << " ]";
}

// file writing operators:
inline std::ostream& operator<<(std::ostream& os, const HoldMap& holdmap)
{
	for (const auto& [pos, regions] : holdmap)
		os << '(' << pos.x() << ',' << pos.y() << ") = " << regions << '\n';
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const Point& p)
{
	return os << p.x() << "," << p.y();
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
