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
	cv::Rect cvRect(compRect.x, compRect.y, compRect.width, compRect.height);
	return imgHistComp(cur(cvRect), m_matMap.at(src)(cvRect));
}

asst::Rect Identify::findImage(const cv::Mat& image, const cv::Mat& templ)
{
	return { 0, 0, 0, 0 };
}