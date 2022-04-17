#pragma once
#include <opencv2/opencv.hpp>

#include <fileutil.hpp>

template<typename ImageType = cv::Mat>
struct ImageWrapper {
	std::string filepath;
	ImageType image;

	ImageWrapper(const std::string& path, const bool& load = true) : filepath{ path }, image{ load ? cv::imread(filepath) : ImageType{} } {}

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
