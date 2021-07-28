#pragma once

#include <unordered_map>
#include <utility>
#include <tuple>
#include <optional>

#include <opencv2/opencv.hpp>
#include <OcrLiteNcnn/OcrLite.h>

#include "AsstDef.h"


namespace asst {

	class WinMacro;

	class Identify
	{
	public:
		Identify() = default;
		~Identify() = default;

		/*** OpenCV package ***/
		void set_use_cache(bool b) noexcept;
		bool add_image(const std::string& name, const std::string& path);

		// return tuple< algorithmType, suitability, matched asst::rect>
		std::tuple<AlgorithmType, double, asst::Rect> find_image(const cv::Mat& image, const std::string& templ, double threshold = 0.99);

		void clear_cache();

		/*** OcrLite package ***/
		void set_ocr_param(int gpu_index, int thread_number);
		bool ocr_init_models(const std::string& dir);
		std::optional<Rect> find_text(const cv::Mat& mat, const std::string& text);

	private:
		cv::Mat image_2_hist(const cv::Mat& src);
		double image_hist_comp(const cv::Mat& src, const cv::MatND& hist);
		static asst::Rect cvrect_2_rect(const cv::Rect& cvRect) { 
			return asst::Rect(cvRect.x, cvRect.y, cvRect.width, cvRect.height); 
		}
		std::vector<TextArea> ocr_detect(const cv::Mat& mat);

		// return pair< suitability, raw opencv::point>
		std::pair<double, cv::Point> match_template(const cv::Mat& cur, const cv::Mat& templ);

		std::unordered_map<std::string, cv::Mat> m_mat_map;
		bool m_use_cache = true;
		std::unordered_map<std::string, std::pair<cv::Rect, cv::Mat>> m_cache_map;	// Œª÷√°¢÷±∑ΩÕºª∫¥Ê

		ocr::OcrLite m_ocr_lite;
	};

}
