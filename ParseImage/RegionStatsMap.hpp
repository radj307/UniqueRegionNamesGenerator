#pragma once 
#include "Region.hpp"
#include "RegionStats.hpp"

#include <map>

struct RegionStatsMap : std::map<Region, RegionStats> {
	using base = std::map<Region, RegionStats>;
	using base::base;

	RegionStats get(const ID& regionID) const
	{
		for (const auto& [region, stats] : *this)
			if (region.getID() == regionID)
				return stats;
		throw make_exception("No region with ID ", regionID, " was found!");
	}
};
