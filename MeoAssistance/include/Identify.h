#pragma once

#include <unordered_map>
#include <utility>
#include <tuple>
#include <opencv2/opencv.hpp>

#include "AsstDef.h"


namespace asst {

	class WinMacro;

	class Identify
	{
	public:
		Identify() = default;
		~Identify() = default;

		void set_use_cache(bool b) noexcept;
		bool add_image(const std::string& name, const std::string& path);

		// return tuple< algorithmType, suitability, scaled asst::rect>
		std::tuple<AlgorithmType, double, asst::Rect> find_image(const cv::Mat& image, const std::string& templ, double threshold = 0.99);

		void clear_cache();
	private:
		cv::Mat image_2_hist(const cv::Mat& src);
		double image_hist_comp(const cv::Mat& src, const cv::MatND& hist);
		static asst::Rect cvrect_2_rect(const cv::Rect& cvRect) { 
			return asst::Rect(cvRect.x, cvRect.y, cvRect.width, cvRect.height); 
		}

		// return pair< suitability, raw opencv::point>
		std::pair<double, cv::Point> find_image(const cv::Mat& cur, const cv::Mat& templ);

		std::unordered_map<std::string, cv::Mat> m_mat_map;
		bool m_use_cache = true;
		std::unordered_map<std::string, std::pair<cv::Rect, cv::Mat>> m_cache_map;	// Œª÷√°¢÷±∑ΩÕºª∫¥Ê
	};

}
