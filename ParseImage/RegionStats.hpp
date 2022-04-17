#pragma once
#include <opencv2/opencv.hpp>

#include <optional>
#include <vector>

struct RegionStats : std::vector<cv::Point> {
	using base = std::vector<cv::Point>;
	using base::base;

	std::optional<int> getTopYPos() const
	{
		std::optional<int> top;
		for (const auto& it : *this) {
			if (!top.has_value())
				top = it.y;
			else if (it.y > top.value())
				top = it.y;
		}
		return top;
	}
	std::optional<int> getBottomYPos() const
	{
		std::optional<int> top;
		for (const auto& it : *this)
			if (!top.has_value() || it.y < top.value())
				top = it.y;
		return top;
	}

	cv::Point get_first_at(const int& y_pos) const
	{
		std::optional<cv::Point> min;
		for (const auto& it : *this) {
			if (it.y == y_pos && (!min.has_value() || it.x < min.value().x))
				min = it;
			else continue;
		}
		return min.value(); // throw exception if failed
	}
	cv::Point get_last_at(const int& y_pos) const
	{
		std::optional<cv::Point> max;
		for (const auto& it : *this) {
			if (it.y == y_pos && (!max.has_value() || it.x > max.value().x))
				max = it;
			else continue;
		}
		return max.value(); // throw exception if failed
	}

	constexpr bool contains(const cv::Point& p) const
	{
		return std::any_of(begin(), end(), [&p](cv::Point const& pos) -> bool { return pos == p; });
	}

	RegionStats filter_region_area() const
	{
		std::vector<cv::Point> vecFirst, vecLast;

		const auto& top{ getTopYPos().value_or(0) }, bottom{ getBottomYPos().value_or(0) };

		vecFirst.reserve(top - bottom);
		vecLast.reserve(top - bottom);

		for (int i{ bottom }; i <= top; ++i) {
			const auto& first{ get_first_at(i) }, last{ get_last_at(i) };
			vecFirst.emplace_back(first);
			vecLast.emplace_back(last);
		}

		using iter = std::vector<cv::Point>::const_iterator;

		const auto& isInnerPoint{ [](const iter& pos, const iter& first, const iter& last) -> bool {
			if (std::distance(pos, first) == 0ull || std::distance(pos, last) <= 1ull)
				return false;
			const auto& prev{ pos - 1 }, & next{ pos + 1 };
			return pos->x == prev->x && pos->x == next->x;
		} };

		// remove unnecessary cv::Points from the vector of first cv::Points
		for (auto it{ vecFirst.begin() }; it != vecFirst.end();) {
			if (isInnerPoint(it, vecFirst.begin(), vecFirst.end()))
				it = vecFirst.erase(it);
			if (it != vecFirst.end()) ++it;
		}
		vecFirst.shrink_to_fit();

		// remove unnecessary cv::Points from the vector of last cv::Points
		for (auto it{ vecLast.begin() }; it != vecLast.end();) {
			if (isInnerPoint(it, vecLast.begin(), vecLast.end()))
				it = vecLast.erase(it);
			if (it != vecLast.end()) ++it;
		}
		vecLast.shrink_to_fit();

		RegionStats edge;
		edge.reserve(vecFirst.size() + vecLast.size());
		// iterate forwards through first cv::Points
		const auto& vecFirstHalfSize{ vecFirst.size() / 2ull }; //< used to calculate padding
		for (auto it{ vecFirst.begin() }, end{ vecFirst.end() }; it != end; ++it) {
			int y_offset{ 0 };
			// if we're in the first half of the vector, shift all Y axis cv::Points up by 1
			if (std::distance(it, vecFirst.begin()) < vecFirstHalfSize)
				--y_offset;
			// else shift all Y axis cv::Points down by 1
			else ++y_offset;
			// shift all first cv::Points left by 1
			edge.emplace_back(cv::Point{ it->x - 1, it->y + y_offset });
		}
		const auto& vecLastHalfSize{ vecLast.size() / 2ull };
		// iterate backwards through last cv::Points to complete polygon
		for (auto it{ vecLast.rbegin() }, end{ vecLast.rend() }; it != end; ++it) {
			int y_offset{ 0 };
			// if we're in the first half of the vector, shift all Y axis cv::Points up by 1
			if (std::distance(it, vecLast.rbegin()) < vecLastHalfSize)
				++y_offset;
			// else shift all Y axis cv::Points down by 1
			else --y_offset;
			// shift all last cv::Points right by 1
			edge.emplace_back(cv::Point{ it->x + 1, it->y + y_offset });
		}
		edge.shrink_to_fit();
		return edge;
	}
};
