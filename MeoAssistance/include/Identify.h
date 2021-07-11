#pragma once

#include "AssDef.h"

#include <opencv2/opencv.hpp>
#include <unordered_map>

namespace MeoAssistance {

	class WinMacro;

	class Identify
	{
	public:
		Identify() = default;
		~Identify() = default;
		double imgHistComp(const cv::Mat& lhs, const cv::Mat& rhs);
		double imgHistComp(const cv::Mat& cur, const std::string& src, MeoAssistance::Rect compRect);
		bool addImage(const std::string& name, const std::string& path);
	private:
		std::unordered_map<std::string, cv::Mat> m_matMap;
	};
}
