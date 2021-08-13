#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <optional>

#include <opencv2/opencv.hpp>
#include <OcrLiteOnnx/OcrLiteCaller.h>

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
		bool add_text_image(const std::string& text, const std::string& path);

		// return tuple< algorithmType, suitability, matched asst::rect>
		std::tuple<AlgorithmType, double, asst::Rect> find_image(const cv::Mat& image, const std::string& templ, double templ_threshold);

		// for debug
		void feature_matching(const cv::Mat& mat);

		void clear_cache();

		/*** OcrLite package ***/
		void set_ocr_param(int gpu_index, int thread_number);
		bool ocr_init_models(const std::string& dir);
		std::optional<Rect> find_text(const cv::Mat& mat, const std::string& text);
		std::vector<TextArea> find_text(const cv::Mat& mat, const std::vector<std::string>& texts);
		std::vector<TextArea> find_text(const cv::Mat& mat, const std::unordered_set<std::string>& texts);

		std::vector<TextArea> ocr_detect(const cv::Mat& mat);
	private:
		cv::Mat image_2_hist(const cv::Mat& src);
		double image_hist_comp(const cv::Mat& src, const cv::MatND& hist);
		static asst::Rect cvrect_2_rect(const cv::Rect& cvRect);

		// return pair<特征点s，特征点描述子（向量）>
		std::pair<std::vector<cv::KeyPoint>, cv::Mat> surf_detect(const cv::Mat& mat);

		// return pair< suitability, raw opencv::point>
		std::pair<double, cv::Point> match_template(const cv::Mat& cur, const cv::Mat& templ);

		std::unordered_map<std::string, cv::Mat> m_mat_map;
		bool m_use_cache = true;
		std::unordered_map<std::string, std::pair<cv::Rect, cv::Mat>> m_cache_map;	// 位置、直方图缓存
		// value: pair<特征点s，特征点描述子（向量）>
		std::unordered_map<std::string, std::pair<std::vector<cv::KeyPoint>, cv::Mat>> m_feature_map;

		OcrLiteCaller m_ocr_lite;
	};
}
