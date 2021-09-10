#include "InfrastAbstractTask.h"

#include <opencv2/opencv.hpp>

#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"
#include "InfrastConfiger.h"

using namespace asst;

asst::InfrastAbstractTask::InfrastAbstractTask(AsstCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg),
	m_swipe_begin(Configer::WindowWidthDefault * 0.9, Configer::WindowHeightDefault * 0.5, 0, 0),
	m_swipe_end(Configer::WindowWidthDefault * 0.5, Configer::WindowHeightDefault * 0.5, 0, 0)
{
}

bool asst::InfrastAbstractTask::swipe_to_the_left()
{
	constexpr int SwipeTimes = 5;

	m_swipe_duration = 100;
	m_swipe_extra_delay = 0;
	// 往左使劲滑几下
	bool ret = false;
	for (int i = 0; i != SwipeTimes; ++i) {
		ret = swipe(true);
		if (!ret) {
			break;
		}
	}
	m_swipe_duration = SwipeDurationDefault;
	m_swipe_extra_delay = SwipeExtraDelayDefault;
	sleep(SwipeExtraDelayDefault);
	return ret;
}

bool asst::InfrastAbstractTask::click_clear_button()
{
	const static Rect ClearButtonRect(430, 655, 150, 40);
	return m_controller_ptr->click(ClearButtonRect);
}

bool asst::InfrastAbstractTask::click_confirm_button()
{
	const static Rect ConfirmButtonRect(1105, 655, 150, 40);
	return m_controller_ptr->click(ConfirmButtonRect);
}

bool asst::InfrastAbstractTask::click_return_button()
{
	const static Rect ConfirmButtonRect(20, 20, 135, 35);
	return m_controller_ptr->click(ConfirmButtonRect);
}

bool asst::InfrastAbstractTask::swipe(bool reverse)
{
//#ifndef LOG_TRACE
	bool ret = true;
	if (!reverse) {
		ret &= m_controller_ptr->swipe(m_swipe_begin, m_swipe_end, m_swipe_duration);
		++m_swipe_times;
	}
	else {
		ret &= m_controller_ptr->swipe(m_swipe_end, m_swipe_begin, m_swipe_duration);
		--m_swipe_times;
	}
	ret &= sleep(m_swipe_extra_delay);
	return ret;
//#else
//	return sleep(SwipeExtraDelay);
//#endif
}

std::vector<TextArea> asst::InfrastAbstractTask::detect_operators_name(
	const cv::Mat& image,
	std::unordered_map<std::string,
	std::string>& feature_cond,
	std::unordered_set<std::string>& feature_whatever)
{
	// 裁剪出来干员名的一个长条形图片，没必要把整张图片送去识别
	int cropped_height = image.rows * m_cropped_height_ratio;
	int cropped_upper_y = image.rows * m_cropped_upper_y_ratio;
	cv::Mat upper_part_name_image = image(cv::Rect(0, cropped_upper_y, image.cols, cropped_height));
	// ocr库，单色图片识别效果好很多；但是只接受三通道的图片，所以这里转两次，送进去单色的、三通道的图片
	cv::cvtColor(upper_part_name_image, upper_part_name_image, cv::COLOR_BGR2GRAY);
	cv::cvtColor(upper_part_name_image, upper_part_name_image, cv::COLOR_GRAY2BGR);

	std::vector<TextArea> upper_text_area = ocr_detect(upper_part_name_image);	// 所有文字
	// 因为图片是裁剪过的，所以对应原图的坐标要加上裁剪的参数
	for (TextArea& textarea : upper_text_area) {
		textarea.rect.y += cropped_upper_y;
	}
	// 过滤出所有的干员名
	std::vector<TextArea> upper_part_names = text_match(
		upper_text_area,
		InfrastConfiger::get_instance().m_all_opers_name,
		Configer::get_instance().m_infrast_ocr_replace);
	// 把这一块涂黑，避免后面被特征检测的误识别了
	for (const TextArea& textarea : upper_part_names) {
		cv::Rect rect(textarea.rect.x, textarea.rect.y - cropped_upper_y, textarea.rect.width, textarea.rect.height);
		// 这里是转过灰度图再转回来的，相当于深拷贝，不会影响原图
		cv::rectangle(upper_part_name_image, rect, cv::Scalar(0, 0, 0), -1);
	}

	// 下半部分的干员
	int cropped_lower_y = image.rows * m_cropped_lower_y_ratio;
	cv::Mat lower_part_name_image = image(cv::Rect(0, cropped_lower_y, image.cols, cropped_height));
	// ocr库，单色图片识别效果好很多；但是只接受三通道的图片，所以这里转两次，送进去单色的、三通道的图片
	cv::cvtColor(lower_part_name_image, lower_part_name_image, cv::COLOR_BGR2GRAY);
	cv::cvtColor(lower_part_name_image, lower_part_name_image, cv::COLOR_GRAY2BGR);

	std::vector<TextArea> lower_text_area = ocr_detect(lower_part_name_image);	// 所有文字
	// 因为图片是裁剪过的，所以对应原图的坐标要加上裁剪的参数
	for (TextArea& textarea : lower_text_area) {
		textarea.rect.y += cropped_lower_y;
	}
	// 过滤出所有的干员名
	std::vector<TextArea> lower_part_names = text_match(
		lower_text_area,
		InfrastConfiger::get_instance().m_all_opers_name,
		Configer::get_instance().m_infrast_ocr_replace);
	// 把这一块涂黑，避免后面被特征检测的误识别了
	for (const TextArea& textarea : lower_part_names) {
		cv::Rect rect(textarea.rect.x, textarea.rect.y - cropped_lower_y, textarea.rect.width, textarea.rect.height);
		// 这里是转过灰度图再转回来的，相当于深拷贝，不会影响原图
		cv::rectangle(lower_part_name_image, rect, cv::Scalar(0, 0, 0), -1);
	}

	// 上下两部分识别结果合并
	std::vector<TextArea> all_text_area = std::move(upper_text_area);
	all_text_area.insert(all_text_area.end(),
		std::make_move_iterator(lower_text_area.begin()),
		std::make_move_iterator(lower_text_area.end()));
	std::vector<TextArea> all_opers_textarea = std::move(upper_part_names);
	all_opers_textarea.insert(all_opers_textarea.end(),
		std::make_move_iterator(lower_part_names.begin()),
		std::make_move_iterator(lower_part_names.end()));

	// 如果ocr结果中已经有某个干员了，就没必要再尝试对他特征检测了，直接删了
	for (const TextArea& textarea : all_opers_textarea) {
		auto cond_iter = std::find_if(feature_cond.begin(), feature_cond.end(),
			[&textarea](const auto& pair) -> bool {
				return textarea.text == pair.second;
			});
		if (cond_iter != feature_cond.end()) {
			feature_cond.erase(cond_iter);
		}

		auto whatever_iter = std::find_if(feature_whatever.begin(), feature_whatever.end(),
			[&textarea](const std::string& str) -> bool {
				return textarea.text == str;
			});
		if (whatever_iter != feature_whatever.end()) {
			feature_whatever.erase(whatever_iter);
		}
	}

	// 用特征检测再筛选一遍OCR识别漏了的——有关键字的
	for (const TextArea& textarea : all_text_area) {
		auto find_iter = std::find_if(all_opers_textarea.cbegin(), all_opers_textarea.cend(),
			[&textarea](const auto& rhs) -> bool {
				return textarea.text == rhs.text; });
		if (find_iter != all_opers_textarea.cend()) {
			continue;
		}
		for (auto iter = feature_cond.begin(); iter != feature_cond.end(); ++iter) {
			auto& [key, value] = *iter;
			// 识别到了key，但是没识别到value，这种情况就需要进行特征检测进一步确认了
			if (textarea.text.find(key) != std::string::npos
				&& textarea.text.find(value) == std::string::npos) {
				// 把key所在的矩形放大一点送去做特征检测，不需要把整张图片都送去检测
				Rect magnified_area = textarea.rect.center_zoom(2.0);
				magnified_area.x = (std::max)(0, magnified_area.x);
				magnified_area.y = (std::max)(0, magnified_area.y);
				if (magnified_area.x + magnified_area.width >= image.cols) {
					magnified_area.width = image.cols - magnified_area.x - 1;
				}
				if (magnified_area.y + magnified_area.height >= image.rows) {
					magnified_area.height = image.rows - magnified_area.y - 1;
				}
				cv::Rect cv_rect(magnified_area.x, magnified_area.y, magnified_area.width, magnified_area.height);
				// key是关键字而已，真正要识别的是value
				auto&& ret = OcrAbstractTask::m_identify_ptr->feature_match(image(cv_rect), value);
				if (ret) {
					// 匹配上了下次就不用再匹配这个了，直接删了
					all_opers_textarea.emplace_back(value, textarea.rect);
					iter = feature_cond.erase(iter);
					--iter;
					// 也从whatever里面删了
					auto whatever_iter = std::find_if(feature_whatever.begin(), feature_whatever.end(),
						[&textarea](const std::string& str) -> bool {
							return textarea.text == str;
						});
					if (whatever_iter != feature_whatever.end()) {
						feature_whatever.erase(whatever_iter);
					}
					// 顺便再涂黑了，避免后面被whatever特征检测的误识别
					// 注意这里是浅拷贝，原图image也会被涂黑
					//cv::rectangle(draw_image, cv_rect, cv::Scalar(0, 0, 0), -1);
				}
			}
		}
	}

	// 用特征检测再筛选一遍OCR识别漏了的——无论如何都进行识别的
	for (auto iter = feature_whatever.begin(); iter != feature_whatever.end(); ++iter) {
		// 上半部分长条形的图片
		auto&& upper_ret = OcrAbstractTask::m_identify_ptr->feature_match(upper_part_name_image, *iter);
		if (upper_ret) {
			TextArea temp = std::move(upper_ret.value());
#ifdef LOG_TRACE	// 也顺便涂黑一下，方便看谁没被识别出来
			cv::Rect draw_rect(temp.rect.x, temp.rect.y, temp.rect.width, temp.rect.height);
			// 注意这里是浅拷贝，原图image也会被涂黑
			cv::rectangle(upper_part_name_image, draw_rect, cv::Scalar(0, 0, 0), -1);
#endif
			// 因为图片是裁剪过的，所以对应原图的坐标要加上裁剪的参数
			temp.rect.y += cropped_upper_y;
			all_opers_textarea.emplace_back(std::move(temp));
			iter = feature_whatever.erase(iter);
			--iter;
			continue;
		}
		// 下半部分长条形的图片
		auto&& lower_ret = OcrAbstractTask::m_identify_ptr->feature_match(lower_part_name_image, *iter);
		if (lower_ret) {
			TextArea temp = std::move(lower_ret.value());
#ifdef LOG_TRACE	// 也顺便涂黑一下，方便看谁没被识别出来
			cv::Rect draw_rect(temp.rect.x, temp.rect.y, temp.rect.width, temp.rect.height);
			// 注意这里是浅拷贝，原图image也会被涂黑
			cv::rectangle(lower_part_name_image, draw_rect, cv::Scalar(0, 0, 0), -1);
#endif
			// 因为图片是裁剪过的，所以对应原图的坐标要加上裁剪的参数
			temp.rect.y += cropped_lower_y;
			all_opers_textarea.emplace_back(std::move(temp));
			iter = feature_whatever.erase(iter);
			--iter;
			continue;
		}
	}

	return all_opers_textarea;
}

bool asst::InfrastAbstractTask::enter_station(const std::vector<std::string>& templ_names, int index, double threshold)
{
	cv::Mat image = m_controller_ptr->get_image();

	std::vector<asst::Identify::FindImageResult> max_size_reslut;
	for (const auto& templ : templ_names) {
		auto temp_result = m_identify_ptr->find_all_images(image, templ, threshold);
		if (temp_result.size() > max_size_reslut.size()) {
			max_size_reslut = temp_result;
		}
	}

	if (max_size_reslut.empty()) {
		return false;
	}
	if (index >= max_size_reslut.size()) {
		return false;
	}

	std::sort(max_size_reslut.begin(), max_size_reslut.end(), [](
		const auto& lhs, const auto& rhs) -> bool {
			if (std::abs(lhs.rect.y - rhs.rect.y) < 5) {	// 同一排的
				return lhs.rect.x < rhs.rect.x;
			}
			else {
				return lhs.rect.y < rhs.rect.y;
			}
		});

	m_controller_ptr->click(max_size_reslut.at(index).rect);
	sleep(1000);

	return false;
}
