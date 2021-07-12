#include "Identify.h"


#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

using namespace asst;
using namespace cv;

bool Identify::addImage(const std::string& name, const std::string& path)
{
	Mat mat = imread(path);
	if (mat.empty()) {
		return false;
	}
	m_matMap.emplace(name, mat);
	return true;
}

double Identify::imgHistComp(const cv::Mat& lhs, const cv::Mat& rhs)
{
	cv::Mat lhs_hsv;
	cv::Mat rhs_hsv;
	cvtColor(lhs, lhs_hsv, COLOR_BGR2HSV);
	cvtColor(rhs, rhs_hsv, COLOR_BGR2HSV);

	int histSize[] = { 50, 60 };
	float h_ranges[] = { 0, 180 };
	float s_ranges[] = { 0, 256 };
	const float* ranges[] = { h_ranges, s_ranges };
	int channels[] = { 0, 1 };

	MatND lhs_hist;
	MatND rhs_hist;

	calcHist(&lhs_hsv, 1, channels, Mat(), lhs_hist, 2, histSize, ranges);
	normalize(lhs_hist, lhs_hist, 0, 1, NORM_MINMAX);

	calcHist(&rhs_hsv, 1, channels, Mat(), rhs_hist, 2, histSize, ranges);
	normalize(rhs_hist, rhs_hist, 0, 1, NORM_MINMAX);

	return compareHist(lhs_hist, rhs_hist, CV_COMP_CORREL);
}

double Identify::imgHistComp(const cv::Mat& cur, const std::string& src, asst::Rect compRect)
{
	if (m_matMap.find(src) == m_matMap.end()) {
		return 0;
	}
	cv::Rect cvRect(compRect.x, compRect.y, compRect.width, compRect.height);
	return imgHistComp(cur(cvRect), m_matMap.at(src)(cvRect));
}

std::pair<double, asst::Rect> Identify::findImage(const cv::Mat& image, const cv::Mat& templ)
{
	cv::Mat image_hsv;
	cv::Mat templ_hsv;
	cvtColor(image, image_hsv, COLOR_BGR2HSV);
	cvtColor(templ, templ_hsv, COLOR_BGR2HSV);

	Mat matched;
	matchTemplate(image_hsv, templ_hsv, matched, cv::TM_CCORR_NORMED);

	double minVal = 0, maxVal = 0;
	cv::Point minLoc, maxLoc;
	minMaxLoc(matched, &minVal, &maxVal, &minLoc, &maxLoc);

	return { maxVal, asst::Rect(maxLoc.x, maxLoc.y, templ.cols, templ.rows).center_zoom(0.8) };
}

std::pair<double, asst::Rect> Identify::findImage(const cv::Mat& cur, const std::string& templ)
{
	if (m_matMap.find(templ) == m_matMap.end()) {
		return { 0, asst::Rect() };
	}
	return findImage(cur, m_matMap.at(templ));
}

std::pair<double, asst::Rect> Identify::findImageWithFile(const cv::Mat& cur, const std::string& filename)
{
	Mat mat = imread(filename);
	if (mat.empty()) {
		return { 0, asst::Rect() };
	}
	return findImage(cur, mat);
}
