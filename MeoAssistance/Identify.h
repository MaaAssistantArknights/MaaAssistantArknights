#pragma once

#include <opencv2/opencv.hpp>

namespace MeoAssistance {

	class WinMacro;

	class Identify
	{
	public:
		Identify() = default;
		~Identify() = default;
		double imgHistComp(const cv::Mat& lhs, const cv::Mat& rhs);
	private:
	};
}
