#pragma once

#include "AsstDef.h"

#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <utility>

namespace asst {

	class WinMacro;

	class Identify
	{
	public:
		Identify() = default;
		~Identify() = default;

		bool addImage(const std::string& name, const std::string& path);

		double imgHistComp(const cv::Mat& lhs, const cv::Mat& rhs);
		double imgHistComp(const cv::Mat& cur, const std::string& src, asst::Rect compRect);
		std::pair<double, asst::Rect> findImage(const cv::Mat& image, const cv::Mat& templ);
		std::pair<double, asst::Rect> findImage(const cv::Mat& cur, const std::string& templ);
		std::pair<double, asst::Rect> findImageWithFile(const cv::Mat& cur, const std::string& filename);

	private:
		std::unordered_map<std::string, cv::Mat> m_matMap;
	};
}
