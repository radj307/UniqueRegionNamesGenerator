#pragma once
#include "Region.hpp"
#include "config.hpp"
#include "TMap.hpp"

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
	cv::Size partSize{ 0, 0 };
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

		if (stats.partSize = { std::forward<cv::Mat>(part).cols, std::forward<cv::Mat>(part).rows }; stats.partSize.width > 0 && stats.partSize.height > 0)
			stats.is_valid = true;
		else return stats;

		if (const auto& channels{ part.channels() }; channels != 3)
			throw make_exception("Loaded image with an incorrect number of color channels!");

		const auto& rows{ part.rows }, & cols{ part.cols };

		for (int y{ 0 }; y < rows; ++y) {
			for (int x{ 0 }; x < cols; ++x) {
				RGB color{ Vec3b_to_RGB(std::move(std::forward<cv::Mat>(part).at<cv::Vec3b>(cv::Point{ x, y }))) };

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
	float getPercentage(Region const& region) const { return static_cast<float>(getCount(region)) / static_cast<float>(partSize.width * partSize.height); }

	/**
	 * @brief				Get the regions present in this partition that are above a specified threshold.
	 * @param threshold		The threshold percentage _( 0.0 - 1.0, using operation `>=` )_ of pixels that a region must have in order to be returned.
	 * @returns				RegionList containing all regions with a higher percentage of pixels than the given threshold.
	 */
	std::vector<Region> getRegions(const float& threshold = 0.0f) const noexcept(false)
	{
		if (threshold < 0.0f || threshold > 1.0f)
			throw make_exception("Invalid threshold value '", threshold, "' is out-of-range: ( 0.0 - 1.0 )!");
		const float totalPixelCount{ static_cast<float>(partSize.width * partSize.height) };
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

