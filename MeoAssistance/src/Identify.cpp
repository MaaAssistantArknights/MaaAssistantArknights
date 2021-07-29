#include "Identify.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include "Logger.hpp"
#include "AsstAux.h"

using namespace asst;
using namespace cv;

bool Identify::add_image(const std::string& name, const std::string& path)
{
	Mat mat = imread(path);
	if (mat.empty()) {
		return false;
	}
	m_mat_map.emplace(name, mat);
	return true;
}

void Identify::set_use_cache(bool b) noexcept
{
	if (b) {
		m_use_cache = true;
	}
	else {
		m_cache_map.clear();
		m_use_cache = false;
	}
}

Mat Identify::image_2_hist(const cv::Mat& src)
{
	Mat src_hsv;
	cvtColor(src, src_hsv, COLOR_BGR2HSV);

	int histSize[] = { 50, 60 };
	float h_ranges[] = { 0, 180 };
	float s_ranges[] = { 0, 256 };
	const float* ranges[] = { h_ranges, s_ranges };
	int channels[] = { 0, 1 };

	MatND src_hist;

	calcHist(&src_hsv, 1, channels, Mat(), src_hist, 2, histSize, ranges);
	normalize(src_hist, src_hist, 0, 1, NORM_MINMAX);

	return src_hist;
}

double Identify::image_hist_comp(const cv::Mat& src, const cv::MatND& hist)
{
	// keep the interface return value unchanged
	return 1 - compareHist(image_2_hist(src), hist, CV_COMP_BHATTACHARYYA);
}

std::vector<TextArea> asst::Identify::ocr_detect(const cv::Mat& mat)
{
	ocr::OcrResult ocr_results = m_ocr_lite.detect(mat,
		50, 0,
		0.6f, 0.3f,
		2.0f, false, false);

	std::vector<TextArea> result;
	for (ocr::TextBlock & text_block : ocr_results.textBlocks) {
		if (text_block.boxPoint.size() != 4) {
			continue;
		}
		// the rect like ↓
		// 0 - 1 
		// 3 - 2
		int x = text_block.boxPoint.at(0).x;
		int y = text_block.boxPoint.at(0).y;
		int width = text_block.boxPoint.at(1).x - x;
		int height = text_block.boxPoint.at(3).y - y;
		result.emplace_back(std::move(text_block.text), x, y, width, height);
	}
	return result;
}

std::pair<double, cv::Point> Identify::match_template(const cv::Mat& image, const cv::Mat& templ)
{
	Mat image_hsv;
	Mat templ_hsv;
	cvtColor(image, image_hsv, COLOR_BGR2HSV);
	cvtColor(templ, templ_hsv, COLOR_BGR2HSV);

	Mat matched;
	matchTemplate(image_hsv, templ_hsv, matched, cv::TM_CCOEFF_NORMED);

	double minVal = 0, maxVal = 0;
	cv::Point minLoc, maxLoc;
	minMaxLoc(matched, &minVal, &maxVal, &minLoc, &maxLoc);
	return { maxVal, maxLoc };
}

std::tuple<AlgorithmType, double, asst::Rect> Identify::find_image(const Mat& cur, const std::string& templ, double threshold)
{
	if (m_mat_map.find(templ) == m_mat_map.cend()) {
		return { AlgorithmType::JustReturn, 0, asst::Rect() };
	}

	// 有缓存，用直方图比较，CPU占用会低很多，但要保证每次按钮图片的位置不变
	if (m_use_cache && m_cache_map.find(templ) != m_cache_map.cend()) {
		const auto& [rect, hist] = m_cache_map.at(templ);
		double value = image_hist_comp(cur(rect), hist);
		return { AlgorithmType::CompareHist, value, cvrect_2_rect(rect).center_zoom(0.8) };
	}
	else {	// 没缓存就模板匹配
		const cv::Mat& templ_mat = m_mat_map.at(templ);
		const auto& [value, point] = match_template(cur, templ_mat);
		cv::Rect raw_rect(point.x, point.y, templ_mat.cols, templ_mat.rows);
		
		if (m_use_cache && value >= threshold) {
			m_cache_map.emplace(templ, std::make_pair(raw_rect, image_2_hist(cur(raw_rect))));
		}

		return { AlgorithmType::MatchTemplate, value, cvrect_2_rect(raw_rect).center_zoom(0.8) };
	}
}

void Identify::clear_cache()
{
	m_cache_map.clear();
}

void asst::Identify::set_ocr_param(int gpu_index, int thread_number)
{
	m_ocr_lite.setGpuIndex(gpu_index);
	m_ocr_lite.setNumThread(thread_number);
}

bool asst::Identify::ocr_init_models(const std::string& dir)
{
	constexpr static const char* DetName = "dbnet_op";
	constexpr static const char* ClsName = "angle_op";
	constexpr static const char* RecName = "crnn_lite_op";
	constexpr static const char* KeysName = "keys.txt";

	return m_ocr_lite.initModels(dir + DetName, dir + ClsName, dir + RecName, dir + KeysName);
}

std::optional<asst::Rect> asst::Identify::find_text(const cv::Mat& mat, const std::string& text)
{
	std::vector<TextArea> results = ocr_detect(mat);
	for (const TextArea& res : results) {
		if (res.text == text) {
			return res.rect;
		}
	}
	return std::nullopt;
}

std::vector<TextArea> asst::Identify::find_text(const cv::Mat& mat, const std::vector<std::string>& texts)
{
	std::vector<TextArea> dst;
	std::vector<TextArea> detect_result = ocr_detect(mat);
	for (TextArea& res : detect_result) {
		for (const std::string& t : texts) {
			if (res.text == t) {
				dst.emplace_back(std::move(res));
			}
		}
	}
	return dst;
}

std::vector<TextArea> asst::Identify::find_text(const cv::Mat& mat, const std::unordered_set<std::string>& texts)
{
	std::vector<TextArea> dst;
	std::vector<TextArea> detect_result = ocr_detect(mat);
	for (TextArea& res : detect_result) {
		DebugTrace("detect", Utf8ToGbk(res.text));
		for (const std::string& t : texts) {
			if (res.text == t) {
				dst.emplace_back(std::move(res));
			}
		}
	}
	return dst;
}

/*
std::pair<double, asst::Rect> Identify::findImageWithFile(const cv::Mat& cur, const std::string& filename)
{
	Mat mat = imread(filename);
	if (mat.empty()) {
		return { 0, asst::Rect() };
	}
	return findImage(cur, mat);
}
*/