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
		struct FindImageResult {
			FindImageResult() = default;
			FindImageResult(AlgorithmType algorithm, double value, asst::Rect rect)
				: algorithm(algorithm), value(value), rect(rect) { ; }
			AlgorithmType algorithm;
			double value = 0.0;
			asst::Rect rect;
		};
	public:
		Identify() = default;
		~Identify() = default;

		/*** OpenCV package ***/
		void set_use_cache(bool b) noexcept;
		bool add_image(const std::string& name, const std::string& path);
		bool add_text_image(const std::string& text, const std::string& path);

		constexpr static double NotAddCache = 999.0;
		FindImageResult find_image(
			const cv::Mat& image, const std::string& templ_name, double add_cache_thres = NotAddCache);
		std::vector<FindImageResult> find_all_images(
			const cv::Mat& image, const std::string& templ_name, double threshold = 0);

		// return pair< suitability, raw opencv::point>
		std::pair<double, cv::Point> match_template(const cv::Mat& cur, const cv::Mat& templ);

		std::optional<TextArea> feature_match(const cv::Mat& mat, const std::string& key);

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
		std::optional<Rect> feature_match(
			const std::vector<cv::KeyPoint>& query_keypoints, const cv::Mat& query_mat_vector,
			const std::vector<cv::KeyPoint>& train_keypoints, const cv::Mat& train_mat_vector
#ifdef LOG_TRACE
			, const cv::Mat query_mat = cv::Mat(), const cv::Mat train_mat = cv::Mat()
#endif
		);

		std::unordered_map<std::string, cv::Mat> m_mat_map;
		bool m_use_cache = true;	// 是否使用缓存——总开关
		std::unordered_map<std::string, std::pair<cv::Rect, cv::Mat>> m_cache_map;	// 位置、直方图缓存
		// value: pair<特征点s，特征点描述子（向量）>
		std::unordered_map<std::string, std::pair<std::vector<cv::KeyPoint>, cv::Mat>> m_feature_map;

		OcrLiteCaller m_ocr_lite;
	};
}
