#pragma once
#include "Region.hpp"

#include <TermAPI.hpp>
#include <ini/MINI.hpp>
#include <color-transform.hpp>

namespace cfg {
	inline RegionVec getRegions(file::MINI const& regions, ushort const& default_priority = 56)
	{
		RegionVec vec;
		vec.reserve(regions.size());

		for (const auto& [edid, sect] : regions) {
			Region reg;
			if (const auto& color{ sect.get("color") }; color.has_value()) {
				if (const auto& hexstr{ color.value() }; hexstr.size() == 6ull && std::all_of(hexstr.begin(), hexstr.end(), str::ishexdigit))
					reg.color = color::hex_to_rgb(hexstr, { 0, 255 });
				else {
					std::cerr << term::get_error() << "Skipping region '" << edid << "' because '" << hexstr << "' isn't a valid 3-channel hexadecimal color value!" << std::endl;
					continue;
				}
			}
			else {
				std::cerr << term::get_warn() << "Skipping region '" << edid << "' because it doesn't specify a color!" << std::endl;
				continue;
			}
			// editor ID
			reg.editorID = edid;
			reg.mapName = sect.get_or("mapName", std::regex_replace(edid, std::basic_regex<char>("([A-Z])"), " $1"));
			reg.priority = sect.getcast_or<ushort>("priority", str::stous, default_priority);
			// add the region to the vector
			vec.emplace_back(std::move(reg));
		}

		vec.shrink_to_fit();
		return vec;
	}
}
