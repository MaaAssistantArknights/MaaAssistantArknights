#include "Identify.h"

#include <algorithm>
#include <numeric>
#include <filesystem>
#include <opencv2/opencv.hpp>

#include <opencv2/xfeatures2d.hpp>
#include <opencv2/imgproc/types_c.h>

#include "Logger.hpp"
#include "AsstAux.h"

using namespace asst;
using namespace cv;
using namespace cv::xfeatures2d;

bool Identify::add_image(const std::string& name, const std::string& path)
{
	Mat image = imread(path);
	if (image.empty()) {
		return false;
	}
	m_mat_map.emplace(name, image);
	return true;
}

bool asst::Identify::add_text_image(const std::string& text, const std::string& path)
{
	Mat image = imread(path);
	if (image.empty()) {
		return false;
	}

	m_feature_map.emplace(text, surf_detect(image));

	return true;
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

std::pair<std::vector<cv::KeyPoint>, cv::Mat> asst::Identify::surf_detect(const cv::Mat& image)
{
	// 灰度图转换
	cv::Mat mat_gray;
	cv::cvtColor(image, mat_gray, cv::COLOR_RGB2GRAY);

	constexpr int min_hessian = 400;
	// SURF特征点检测
	static cv::Ptr<SURF> detector = SURF::create(min_hessian);
	std::vector<KeyPoint> keypoints;
	cv::Mat mat_vector;
	// 找到特征点并计算特征描述子(向量)
	detector->detectAndCompute(mat_gray, Mat(), keypoints, mat_vector);

	return std::make_pair(std::move(keypoints), std::move(mat_vector));
}

std::optional<asst::Rect> asst::Identify::feature_match(
	const std::vector<cv::KeyPoint>& query_keypoints, const cv::Mat& query_mat_vector,
	const std::vector<cv::KeyPoint>& train_keypoints, const cv::Mat& train_mat_vector
#ifdef LOG_TRACE
	, const cv::Mat query_mat, const cv::Mat train_mat
#endif
)
{
	if (query_mat_vector.empty() || train_mat_vector.empty()) {
		DebugTraceError("feature_match | image empty");
		return std::nullopt;
	}
	std::vector<cv::DMatch> matches;
	static FlannBasedMatcher matcher;
	matcher.match(query_mat_vector, train_mat_vector, matches);

#ifdef LOG_TRACE
	//std::cout << matches.size() << " / " << query_keypoints.size() << std::endl;
	cv::Mat allmatch_mat;
	cv::drawMatches(query_mat, query_keypoints, train_mat, train_keypoints, matches, allmatch_mat);
#endif

	// 最大的距离
	auto max_iter = std::max_element(matches.cbegin(), matches.cend(),
		[](const cv::DMatch& lhs, const cv::DMatch& rhs) ->bool {
			return lhs.distance < rhs.distance;
		});	// 描述符欧式距离（knn）
	if (max_iter == matches.cend()) {
		return std::nullopt;;
	}
	float maxdist = max_iter->distance;

	std::vector<cv::DMatch> approach_matches;
	std::vector<cv::KeyPoint> train_approach_keypoints;
	std::vector<cv::KeyPoint> query_approach_keypoints;
	std::vector<cv::Point> train_approach_points;
	std::vector<cv::Point> query_approach_points;
	// 利用距离进行一次逼近
	constexpr static const double MatchRatio = 0.4;
	int approach_index = 0;
	for (const cv::DMatch dmatch : matches) {
		if (dmatch.distance < maxdist * MatchRatio) {
			// 按理说不会越界，以防万一还是检查一下
			if (dmatch.queryIdx >= 0 && dmatch.queryIdx < query_keypoints.size()
				&& dmatch.trainIdx >= 0 && dmatch.trainIdx < train_keypoints.size()) {
				approach_matches.emplace_back(dmatch);
				train_approach_points.emplace_back(train_keypoints.at(dmatch.trainIdx).pt);
				query_approach_points.emplace_back(query_keypoints.at(dmatch.queryIdx).pt);
				train_approach_keypoints.emplace_back(train_keypoints.at(dmatch.trainIdx));
				query_approach_keypoints.emplace_back(query_keypoints.at(dmatch.queryIdx));
			}
		}
	}

#ifdef LOG_TRACE
	cv::Mat approach_mat;
	cv::drawMatches(query_mat, query_keypoints, train_mat, train_keypoints, approach_matches, approach_mat);
#endif

	if (query_approach_points.empty())
	{
		return std::nullopt;
	}

	// 使用RANSAC剔除异常值
	std::vector<uchar> ransac_status;
	cv::Mat fundametal = cv::findFundamentalMat(query_approach_points, train_approach_points, ransac_status, cv::FM_RANSAC);

	std::vector<cv::DMatch> ransac_matchs;
	std::vector<cv::KeyPoint> train_ransac_keypoints;
	std::vector<cv::KeyPoint> query_ransac_keypoints;
	int index = 0;
	for (size_t i = 0; i != ransac_status.size(); ++i) {
		if (ransac_status.at(i) != 0) {
			train_ransac_keypoints.emplace_back(train_approach_keypoints.at(i));
			query_ransac_keypoints.emplace_back(query_approach_keypoints.at(i));
			cv::DMatch dmatch = approach_matches.at(i);
			ransac_matchs.emplace_back(std::move(dmatch));
			++index;
		}
	}

	// 做一次算数均值滤波，过滤异常的点。这个算法有点蠢，TODO可以看下怎么改
	size_t point_size = train_ransac_keypoints.size();
	if (point_size == 0) {
		return std::nullopt;
	}
	cv::Point sum_point = std::accumulate(
		train_ransac_keypoints.cbegin(), train_ransac_keypoints.cend(), cv::Point(),
		[](cv::Point sum, const cv::KeyPoint& rhs) -> cv::Point {
			return cv::Point(sum.x + rhs.pt.x, sum.y + rhs.pt.y);
		});
	cv::Point avg_point(sum_point.x / point_size, sum_point.y / point_size);
	std::vector<cv::DMatch> good_matchs;
	std::vector<cv::Point> good_points;
	// TODO，这个阈值需要根据分辨率进行缩放，而且最好写到配置文件里
	constexpr static int DistanceThreshold = 100;
	for (size_t i = 0; i != train_ransac_keypoints.size(); ++i) {
		// 没必要算距离，x y各算一下就行了，省点CPU时间
		//int distance = std::sqrt(std::pow(avg_point.x - cur_x, 2) + std::pow(avg_point.y - cur_y, 2));
		cv::Point2f& pt = train_ransac_keypoints.at(i).pt;
		int x_distance = std::abs(avg_point.x - pt.x);
		int y_distance = std::abs(avg_point.y - pt.y);
		if (x_distance < DistanceThreshold && y_distance < DistanceThreshold) {
			good_matchs.emplace_back(ransac_matchs.at(i));
			good_points.emplace_back(pt);
		}
	}

#ifdef LOG_TRACE

	cv::Mat ransac_mat;
	cv::drawMatches(query_mat, query_keypoints, train_mat, train_keypoints, ransac_matchs, ransac_mat);

	cv::Mat good_mat;
	cv::drawMatches(query_mat, query_keypoints, train_mat, train_keypoints, good_matchs, good_mat);

#endif

	constexpr static const double MatchSizeRatioThreshold = 0.1;
	if (good_points.size() >= query_keypoints.size() * MatchSizeRatioThreshold) {
		Rect dst;
		int left = 0, right = 0, top = 0, bottom = 0;
		for (const cv::Point& pt : good_points) {
			if (pt.x < left || left == 0) {
				left = pt.x;
			}
			if (pt.x > right || right == 0) {
				right = pt.x;
			}
			if (pt.y < top || top == 0) {
				top = pt.y;
			}
			if (pt.y > bottom || bottom == 0) {
				bottom = pt.y;
			}
		}
		dst = { left, top, right - left, bottom - top };
		return dst;
	}
	return std::nullopt;
}

std::vector<TextArea> asst::Identify::ocr_detect(const cv::Mat& image)
{
	OcrResult ocr_results = m_ocr_lite.detect(image,
		50, 0,
		0.2f, 0.3f,
		2.0f, false, false);

	std::vector<TextArea> result;
	for (TextBlock& text_block : ocr_results.textBlocks) {
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

asst::Identify::FindImageResult asst::Identify::find_image(
	const cv::Mat& image, const std::string& templ_name, double add_cache_thres, bool rect_zoom)
{
	if (m_mat_map.find(templ_name) == m_mat_map.cend()) {
		return { AlgorithmType::JustReturn, 0, Rect() };
	}

	// 有缓存，用直方图比较，CPU占用会低很多，但要保证每次按钮图片的位置不变
	if (m_cache_map.find(templ_name) != m_cache_map.cend()) {
		const auto& [raw_rect, hist] = m_cache_map.at(templ_name);
		double value = image_hist_comp(image(raw_rect), hist);
		Rect dst_rect = make_rect<asst::Rect>(raw_rect);
		if (rect_zoom) {
			dst_rect = dst_rect.center_zoom(0.8);
		}
		return { AlgorithmType::CompareHist, value, dst_rect };
	}
	else {	// 没缓存就模板匹配
		const cv::Mat& templ_mat = m_mat_map.at(templ_name);
		const auto& [value, point] = match_template(image, templ_mat);
		cv::Rect raw_rect(point.x, point.y, templ_mat.cols, templ_mat.rows);
		if (value >= add_cache_thres) {
			m_cache_map.emplace(templ_name, std::make_pair(raw_rect, image_2_hist(image(raw_rect))));
		}
		Rect dst_rect = make_rect<asst::Rect>(raw_rect);
		if (rect_zoom) {
			dst_rect = dst_rect.center_zoom(0.8);
		}
		return { AlgorithmType::MatchTemplate, value, dst_rect };
	}
}

std::vector<asst::Identify::FindImageResult> asst::Identify::find_all_images(
	const cv::Mat& image, const std::string& templ_name, double threshold, bool rect_zoom) const
{
	if (m_mat_map.find(templ_name) == m_mat_map.cend()) {
		return std::vector<FindImageResult>();
	}
	const cv::Mat& templ_mat = m_mat_map.at(templ_name);

	Mat image_hsv;
	Mat templ_hsv;
	cvtColor(image, image_hsv, COLOR_BGR2HSV);
	cvtColor(templ_mat, templ_hsv, COLOR_BGR2HSV);

	Mat matched;
	matchTemplate(image_hsv, templ_hsv, matched, cv::TM_CCOEFF_NORMED);

	std::vector<FindImageResult> results;
	int mini_distance = (std::min)(templ_mat.cols, templ_mat.rows) * 0.5;
	for (int i = 0; i != matched.rows; ++i) {
		for (int j = 0; j != matched.cols; ++j) {
			auto value = matched.at<float>(i, j);
			if (value >= threshold) {
				Rect rect = Rect(j, i, templ_mat.cols, templ_mat.rows);
				if (rect_zoom) {
					rect = rect.center_zoom(0.8);
				}

				bool need_push = true;
				// 如果有两个点离得太近，只取里面得分高的那个
				// 一般相邻的都是刚刚push进去的，这里倒序快一点
				for (auto iter = results.rbegin(); iter != results.rend(); ++iter) {
					if (std::abs(j - iter->rect.x) < mini_distance
						&& std::abs(i - iter->rect.y) < mini_distance) {
						if (iter->score < value) {
							iter->rect = rect;
							iter->score = value;
						}	// else 这个点就放弃了
						need_push = false;
						break;
					}
				}
				if (need_push) {
					results.emplace_back(AlgorithmType::MatchTemplate, value, std::move(rect));
				}
			}
		}
	}

	std::sort(results.begin(), results.end(), [](const auto& lhs, const auto& rhs) -> bool {
		return lhs.score > rhs.score;
		});

#ifdef LOG_TRACE
	cv::Mat draw_mat = image.clone();
	for (const auto& info : results) {
		cv::rectangle(draw_mat, make_rect<cv::Rect>(info.rect), cv::Scalar(0, 0, 255), 1);
		cv::putText(draw_mat, std::to_string(info.score), cv::Point(info.rect.x, info.rect.y), 1, 1.0, cv::Scalar(0, 0, 255));
	}
#endif

	return results;
}

std::optional<TextArea> asst::Identify::feature_match(const cv::Mat& image, const std::string& templ_name)
{
	//DebugTraceFunction;

	if (m_feature_map.find(templ_name) == m_feature_map.cend()) {
		return std::nullopt;
	}
	auto&& [query_keypoints, query_mat_vector] = m_feature_map[templ_name];
	auto&& [train_keypoints, train_mat_vector] = surf_detect(image);

#ifdef LOG_TRACE
	cv::Mat query_mat = cv::imread(GetResourceDir() + "operators\\" + Utf8ToGbk(templ_name) + ".png");
	auto&& ret = feature_match(query_keypoints, query_mat_vector, train_keypoints, train_mat_vector,
		query_mat, image);
#else
	auto&& ret = feature_match(query_keypoints, query_mat_vector, train_keypoints, train_mat_vector);
#endif
	if (ret) {
		TextArea dst;
		dst.text = templ_name;
		dst.rect = std::move(ret.value());
		return dst;
	}
	else {
		return std::nullopt;
	}
}

void Identify::clear_cache()
{
	m_cache_map.clear();
}

// gpu_index是ncnn框架的参数，现在换了onnx的，已经没有这个参数了，但是为了保持接口一致性，保留这个参数，实际不起作用
void asst::Identify::set_ocr_param(int gpu_index, int number_thread)
{
	m_ocr_lite.setNumThread(number_thread);
}

bool asst::Identify::ocr_init_models(const std::string& dir)
{
	constexpr static const char* DetName = "dbnet.onnx";
	constexpr static const char* ClsName = "angle_net.onnx";
	constexpr static const char* RecName = "crnn_lite_lstm.onnx";
	constexpr static const char* KeysName = "keys.txt";

	const std::string dst_filename = dir + DetName;
	const std::string cls_filename = dir + ClsName;
	const std::string rec_filename = dir + RecName;
	const std::string keys_filename = dir + KeysName;

	if (std::filesystem::exists(dst_filename)
		&& std::filesystem::exists(cls_filename)
		&& std::filesystem::exists(rec_filename)
		&& std::filesystem::exists(keys_filename))
	{
		m_ocr_lite.initModels(dst_filename, cls_filename, rec_filename, keys_filename);
		return true;
	}
	return false;
}

std::optional<asst::Rect> asst::Identify::find_text(const cv::Mat& image, const std::string& text)
{
	std::vector<TextArea> results = ocr_detect(image);
	for (const TextArea& res : results) {
		if (res.text == text) {
			return res.rect;
		}
	}
	return std::nullopt;
}

std::vector<TextArea> asst::Identify::find_text(const cv::Mat& image, const std::vector<std::string>& texts)
{
	std::vector<TextArea> dst;
	std::vector<TextArea> detect_result = ocr_detect(image);
	for (TextArea& res : detect_result) {
		for (const std::string& t : texts) {
			if (res.text == t) {
				dst.emplace_back(std::move(res));
			}
		}
	}
	return dst;
}

std::vector<TextArea> asst::Identify::find_text(const cv::Mat& image, const std::unordered_set<std::string>& texts)
{
	std::vector<TextArea> dst;
	std::vector<TextArea> detect_result = ocr_detect(image);
	for (TextArea& res : detect_result) {
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
	Mat image = imread(filename);
	if (image.empty()) {
		return { 0, asst::Rect() };
	}
	return findImage(cur, image);
}
*/