#pragma once

#include "AsstDef.h"

#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <utility>
#include <tuple>

namespace asst {

	class WinMacro;

	class Identify
	{
	public:
		Identify() = default;
		~Identify() = default;

		void setUseCache(bool b) noexcept;
		bool addImage(const std::string& name, const std::string& path);

		// return tuple< algorithmType, suitability, scaled asst::rect>
		std::tuple<int, double, asst::Rect> findImage(const cv::Mat& image, const std::string& templ, double threshold = 0.99);

	private:
		cv::Mat image2Hist(const cv::Mat& src);
		double imageHistComp(const cv::Mat& src, const cv::MatND& hist);
		asst::Rect cvRect2Rect(const cv::Rect& cvRect) { 
			return asst::Rect(cvRect.x, cvRect.y, cvRect.width, cvRect.height); 
		}

		// return pair< suitability, raw opencv::point>
		std::pair<double, cv::Point> findImage(const cv::Mat& cur, const cv::Mat& templ);

		std::unordered_map<std::string, cv::Mat> m_matMap;
		bool m_use_cache = true;
		std::unordered_map<std::string, std::pair<cv::Rect, cv::Mat>> m_cacheMap;
	};

}
