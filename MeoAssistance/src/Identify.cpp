#include "Identify.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

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

Mat Identify::image2hist(const cv::Mat& src)
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
	return 1 - compareHist(image2hist(src), hist, CV_COMP_BHATTACHARYYA);
}

std::pair<double, cv::Point> Identify::find_image(const cv::Mat& image, const cv::Mat& templ)
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

	if (m_use_cache && m_cache_map.find(templ) != m_cache_map.cend()) {
		auto&& [rect, hist] = m_cache_map.at(templ);
		double value = image_hist_comp(cur(rect), hist);
		return { AlgorithmType::CompareHist, value, cvrect2rect(rect).center_zoom(0.8) };
	}
	else {
		auto&& templ_mat = m_mat_map.at(templ);
		auto&& [value, point] = find_image(cur, templ_mat);
		cv::Rect raw_rect(point.x, point.y, templ_mat.cols, templ_mat.rows);
		
		if (m_use_cache && value >= threshold) {
			m_cache_map.emplace(templ, std::make_pair(raw_rect, image2hist(cur(raw_rect))));
		}

		return { AlgorithmType::MatchTemplate, value, cvrect2rect(raw_rect).center_zoom(0.8) };
	}
}

void Identify::clear_cache()
{
	m_cache_map.clear();
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