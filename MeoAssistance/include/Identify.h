#pragma once

#include "AsstDef.h"

#include <opencv2/opencv.hpp>
#include <unordered_map>

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
		Rect findImage(const cv::Mat& image, const cv::Mat& templ);
	private:
		std::unordered_map<std::string, cv::Mat> m_matMap;
	};
}
