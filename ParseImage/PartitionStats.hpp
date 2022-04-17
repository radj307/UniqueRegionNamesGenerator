#pragma once
#include "types.hpp"

#include <color-transform.hpp>

#include <unordered_map>

/**
 * @brief		Convert from OpenCV's BGR pixel color format to RGB.
 * @param bgr	An OpenCV color, with channels ordered Blue-Green-Red (default).
 * @returns		color::RGB<uchar>
 */
inline color::RGB<uchar> Vec3b_to_RGB(cv::Vec3b&& bgr) { return{ std::move(bgr[2]), std::move(bgr[1]), std::move(bgr[0]) }; }

struct PartitionStats {
	using count = unsigned;
private:
	Size partSize{ 0, 0 };
	std::map<Region, count> pxCount;
	bool is_valid{ false };

	/**
	 * @brief			Parse an image partition using the given `ColorMap`, and save the results internally.
	 * @param part		An rvalue of the image partition to parse.
	 * @param colormap	Reference of the `ColorMap` to use when checking pixels.
	 */
	static PartitionStats parse(cv::Mat&& part, const ColorMap& colormap) noexcept(false)
	{
		PartitionStats stats;

		if (stats.partSize = { std::forward<cv::Mat>(part).cols, std::forward<cv::Mat>(part).rows }; stats.partSize.width() > 0 && stats.partSize.height() > 0)
			stats.is_valid = true;
		else return stats;

		if (const auto& channels{ part.channels() }; channels != 3)
			throw make_exception("Loaded image with an incorrect number of color channels!");

		const auto& rows{ part.rows }, & cols{ part.cols };

		for (int y{ 0 }; y < rows; ++y) {
			for (int x{ 0 }; x < cols; ++x) {
				RGB color{ Vec3b_to_RGB(std::move(std::forward<cv::Mat>(part).at<cv::Vec3b>(Point{ x, y }))) };

				if (const auto& mapped{ colormap.find(color) }; mapped != colormap.end())
					++stats.pxCount[mapped->second];
				else continue;
			}
		}

		return stats;
	}

public:
	/// @brief	Default Constructor.
	PartitionStats() = default;
	/**
	 * @brief			Constructor that calls the `parse()` function automatically. Documentation for `parse()`:
	 *\n				Parse an image partition using the given `ColorMap`, and save the results internally.
	 * @param part		The image partition to parse.
	 * @param colormap	Reference of the `ColorMap` to use when checking pixels.
	 */
	PartitionStats(cv::Mat&& part, const ColorMap& colormap)
	{
		*this = PartitionStats::parse(std::forward<cv::Mat>(part), colormap);
	}

	/**
	 * @brief		Check if the partition was valid (not empty).
	 * @returns		true when the partition was valid.
	 */
	bool valid() const { return is_valid; }

	/**
	 * @brief		Check if the partition contains any regions.
	 * @attention	This will always return true if the partition isn't valid, be sure to check that as well!
	 * @returns		true when the partition doesn't contain any recognized region colors.
	 */
	bool empty() const { return pxCount.empty(); }

	/// @brief	Get an iterator to the beginning of the `pxCount` container.
	auto begin() const { return pxCount.begin(); }
	/// @brief	Get an iterator to the end of the `pxCount` container.
	auto end() const { return pxCount.end(); }

	/**
	 * @brief			Check if the partition contains a region.
	 * @param region	The `Region` to check for.
	 * @returns			true when the partition DOES contain the given region.
	 */
	bool contains(Region const& region) const { return pxCount.contains(region); }

	/**
	 * @brief			Get the number of pixels in the partition that match the specified region.
	 * @param region	The `Region` to check for.
	 * @returns			count
	 */
	count getCount(Region const& region) const
	{
		if (const auto& it{ pxCount.find(region) }; it != pxCount.end())
			return it->second;
		else return 0u;
	}

	/**
	 * @brief			Get the percentage of pixels in the partition that match the given region.
	 * @param region	The `Region` to check for.
	 * @returns			float between 0.0 (0%) and 1.0 (100%)
	 */
	float getPercentage(Region const& region) const { return static_cast<float>(getCount(region)) / static_cast<float>(partSize.width() * partSize.height()); }

	/**
	 * @brief				Get the regions present in this partition that are above a specified threshold.
	 * @param threshold		The threshold percentage _( 0.0 - 1.0, using operation `>=` )_ of pixels that a region must have in order to be returned.
	 * @returns				RegionList containing all regions with a higher percentage of pixels than the given threshold.
	 */
	std::vector<Region> getRegions(const float& threshold = 0.0f) const noexcept(false)
	{
		if (threshold < 0.0f || threshold > 1.0f)
			throw make_exception("Invalid threshold value '", threshold, "' is out-of-range: ( 0.0 - 1.0 )!");
		const float totalPixelCount{ static_cast<float>(partSize.width() * partSize.height()) };
		std::vector<Region> vec;
		vec.reserve(pxCount.size());
		for (const auto& [region, count] : pxCount)
			if ((static_cast<float>(count) / totalPixelCount) >= threshold)
				vec.emplace_back(region);
		vec.shrink_to_fit();
		return vec;
	}

	/**
	 * @brief	Retrieve a list of every region with at least one matching pixel present in the partition.
	 * @returns RegionList
	 */
	std::vector<Region> getAllRegions() const
	{
		std::vector<Region> vec;
		vec.reserve(pxCount.size());
		for (const auto& [region, _] : pxCount)
			vec.emplace_back(region);
		vec.shrink_to_fit();
		return vec;
	}
};

struct RegionStats : std::vector<Point> {
	using base = std::vector<Point>;
	using base::base;

	std::optional<position> getTopYPos() const
	{
		std::optional<position> top;
		for (const auto& it : *this) {
			if (!top.has_value())
				top = it.y();
			else if (it.y() > top.value())
				top = it.y();
		}
		return top;
	}
	std::optional<position> getBottomYPos() const
	{
		std::optional<position> top;
		for (const auto& it : *this)
			if (!top.has_value() || it.y() < top.value())
				top = it.y();
		return top;
	}

	Point get_first_at(const position& y_pos) const
	{
		std::optional<Point> min;
		for (const auto& it : *this) {
			const auto& [x, y] { static_cast<std::pair<position, position>>(it) };
			if (y == y_pos && (!min.has_value() || x < min.value().x()))
				min = it;
			else continue;
		}
		return min.value(); // throw exception if failed
	}
	Point get_last_at(const position& y_pos) const
	{
		std::optional<Point> max;
		for (const auto& it : *this) {
			const auto& [x, y] { static_cast<std::pair<position, position>>(it) };
			if (y == y_pos && (!max.has_value() || x > max.value().x()))
				max = it;
			else continue;
		}
		return max.value(); // throw exception if failed
	}

	constexpr bool contains(const Point& p) const
	{
		return std::any_of(begin(), end(), [&p](Point const& pos) -> bool { return pos == p; });
	}

	RegionStats filter_region_area() const
	{
		std::vector<Point> vecFirst, vecLast;

		const auto& top{ getTopYPos().value_or(0) }, bottom{ getBottomYPos().value_or(0) };

		vecFirst.reserve(top - bottom);
		vecLast.reserve(top - bottom);

		for (position i{ bottom }; i <= top; ++i) {
			const auto& first{ get_first_at(i) }, last{ get_last_at(i) };
			vecFirst.emplace_back(first);
			vecLast.emplace_back(last);
		}

		using iter = std::vector<Point>::const_iterator;

		const auto& isInnerPoint{ [](const iter& pos, const iter& first, const iter& last) -> bool {
			if (std::distance(pos, first) == 0ull || std::distance(pos, last) <= 1ull)
				return false;
			const auto& prev{ pos - 1 }, & next{ pos + 1 };
			return pos->x() == prev->x() && pos->x() == next->x();
		} };

		// remove unnecessary points from the vector of first points
		for (auto it{ vecFirst.begin() }; it != vecFirst.end();) {
			if (isInnerPoint(it, vecFirst.begin(), vecFirst.end()))
				it = vecFirst.erase(it);
			if (it != vecFirst.end()) ++it;
		}
		vecFirst.shrink_to_fit();

		// remove unnecessary points from the vector of last points
		for (auto it{ vecLast.begin() }; it != vecLast.end();) {
			if (isInnerPoint(it, vecLast.begin(), vecLast.end()))
				it = vecLast.erase(it);
			if (it != vecLast.end()) ++it;
		}
		vecLast.shrink_to_fit();

		RegionStats edge;
		edge.reserve(vecFirst.size() + vecLast.size());
		// iterate forwards through first points
		const auto& vecFirstHalfSize{ vecFirst.size() / 2ull }; //< used to calculate padding
		for (auto it{ vecFirst.begin() }, end{ vecFirst.end() }; it != end; ++it) {
			position y_offset{ 0 };
			// if we're in the first half of the vector, shift all Y axis points up by 1
			if (std::distance(it, vecFirst.begin()) < vecFirstHalfSize)
				--y_offset;
			// else shift all Y axis points down by 1
			else ++y_offset;
			// shift all first points left by 1
			edge.emplace_back(Point{ it->x() - 1, it->y() + y_offset });
		}
		const auto& vecLastHalfSize{ vecLast.size() / 2ull };
		// iterate backwards through last points to complete polygon
		for (auto it{ vecLast.rbegin() }, end{ vecLast.rend() }; it != end; ++it) {
			position y_offset{ 0 };
			// if we're in the first half of the vector, shift all Y axis points up by 1
			if (std::distance(it, vecLast.rbegin()) < vecLastHalfSize)
				++y_offset;
			// else shift all Y axis points down by 1
			else --y_offset;
			// shift all last points right by 1
			edge.emplace_back(Point{ it->x() + 1, it->y() + y_offset });
		}
		edge.shrink_to_fit();
		return edge;
	}
};

struct RegionStatsMap : std::map<Region, RegionStats> {
	using base = std::map<Region, RegionStats>;
	using base::base;

	RegionStats get(const ID& regionID) const
	{
		for (const auto& [region, stats] : *this)
			if (region.ID() == regionID)
				return stats;
		throw make_exception("No region with ID ", regionID, " was found!");
	}
};
