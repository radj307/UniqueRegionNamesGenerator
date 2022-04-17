#pragma once
#include <fileo.hpp>
#include <fileutil.hpp>
#include <color-transform.hpp>

#include <opencv2/opencv.hpp>

#include <map>
#include <vector>
#include <concepts>

using position = long long;
using length = long long;

/// @brief	RGB color.
using RGB = color::RGB<uchar>;

// forward declare
template<std::integral T> struct basic_point;
template<std::integral T> struct basic_size;
template<std::integral T> struct basic_rectangle;

// 2D point type
template<std::integral T>
struct basic_point : std::pair<T, T> {
	using base = std::pair<T, T>;

	constexpr basic_point(const T& x, const T& y) : base(x, y) {}

	constexpr T& x() { return this->first; }
	constexpr T x() const { return this->first; }

	constexpr T& y() { return this->second; }
	constexpr T y() const { return this->second; }

	basic_point<T>& operator=(const basic_point<T>& o)
	{
		x() = o.x();
		y() = o.y();
		return *this;
	}

	// conversion operator
	operator basic_size<T>() const { return{ x(), y() }; }
	operator cv::Point() const { return cv::Point(x(), y()); }
};
using Point = basic_point<position>;

// 2D size type
template<std::integral T>
struct basic_size : std::pair<T, T> {
	using base = std::pair<T, T>;

	constexpr basic_size(const T& x, const T& y) : base(x, y) {}

	constexpr T& width() { return this->first; }
	constexpr T width() const { return this->first; }

	constexpr T& height() { return this->second; }
	constexpr T height() const { return this->second; }

	using base::operator=;

	basic_size(cv::Size sz) : base(sz.width, sz.height) {}

	basic_size<T>& operator=(const basic_size<T>& o)
	{
		width() = o.width();
		height() = o.height();
		return *this;
	}

	// conversion operator
	operator basic_point<T>() const { return{ width(), height() }; }
	operator cv::Size() const { return cv::Size(width(), height()); }
};
using Size = basic_size<position>;

template<std::integral T>
struct basic_rectangle : basic_point<T>, basic_size<T> {
	using basepoint = basic_point<T>;
	using basesize = basic_size<T>;

	basic_rectangle(const basepoint& pos, const basesize& size) : basepoint(pos), basesize(size) {}
	basic_rectangle(const T& x, const T& y, const T& width, const T& height) : basepoint(x, y), basesize(width, height) {}
	basic_rectangle(cv::Rect rect) : basepoint(rect.x, rect.y), basesize(rect.width, rect.height) {}

	operator cv::Rect() const { return cv::Rect{ static_cast<int>(this->x()), static_cast<int>(this->y()), static_cast<int>(this->width()), static_cast<int>(this->height()) }; }
};
using Rectangle = basic_rectangle<position>;

template<typename ImageType = cv::Mat>
struct Image {
	std::string filepath;
	ImageType image;

	Image(const std::string& path, const bool& load = true) : filepath{ path }, image{ load ? cv::imread(filepath) : ImageType{} } {}

	bool exists() const { return file::exists(filepath); }
	bool loaded() const { return !image.empty(); }

	void openDisplay() const
	{
		cv::namedWindow(filepath);

		cv::imshow(filepath, image);
	}
	void closeDisplay() const
	{
		cv::destroyWindow(filepath);
	}
};

using ID = unsigned short;

struct Region : std::pair<ID, std::string> {
	using base = std::pair<ID, std::string>;
	using base::base;

	constexpr Region() : base(static_cast<::ID>(-1), "") {}

	ID ID() const { return first; }
	std::string Name() const { return second; }

	constexpr bool operator==(const Region& o) const { return first == o.first; }
	constexpr bool operator!=(const Region& o) const { return first != o.first; }

	friend std::ostream& operator<<(std::ostream& os, const Region& region) { return os << region.second; }
};

/// @brief	A map where the keys are `RGB` colors, and the values are `Region` enum types.
using ColorMap = std::map<RGB, Region>;
///// @brief	A vector of pairs where the first element is a `Point` and the second is a vector of `Region` enum types. This is used as an intermediary type between the raw input image, and the output file.
using HoldMap = std::vector<std::pair<Point, std::vector<Region>>>;
