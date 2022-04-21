#pragma once
#include <color-transform.hpp>

#include <string>
#include <vector>
#include <ostream>

using uchar = unsigned char;
using ushort = unsigned short;
using ID = unsigned int;

/// @brief	RGB color.
using RGB = color::RGB<uchar>;

/**
 * @struct	Region
 * @brief	Container object for 1 region's entry in the input file.
 */
struct Region {
private:
	static ID lastID;
public:
	ID id;
	std::string editorID;
	std::string mapName;
	ushort priority{ 0 };
	RGB color;

	Region() : id{ lastID++ } {}
	Region(std::string const& edid, std::string const& mapName, RGB const& rgb, ushort const& priority) :
		id{ lastID++ },
		editorID{ edid },
		mapName{ mapName },
		color{ rgb },
		priority{ priority } {}

	ID getID() const { return id; }
	std::string Name() const { return editorID; }

	operator ID() const { return id; }

	friend std::ostream& operator<<(std::ostream& os, const Region& region)
	{
		return os << region.editorID;
	}
};
inline ID Region::lastID{ 0 };

/// @brief	Region vector
using RegionVec = std::vector<Region>;

/**
 * @brief				Stream writing operator for the RegionVec type.
 * @param os			Output stream to write to.
 * @param regionList	RegionVec to write.
 * @returns				std::ostream&
 */
inline std::ostream& operator<<(std::ostream& os, const RegionVec& regionList)
{
	os << "[ ";
	for (auto it{ regionList.begin() }, endit{ regionList.end() }; it != endit; ++it) {
		os << '"' << *it << '"';
		if (std::distance(it, regionList.end()) > 1ull)
			os << ", ";
	}
	return os << " ]";
}
