#include "InfrastDormTask.h"

#include <future>
#include <thread>

#include <opencv2/opencv.hpp>

#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"

using namespace asst;

asst::InfrastDormTask::InfrastDormTask(AsstCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg)
{
	;
}

bool asst::InfrastDormTask::run()
{
	if (m_view_ptr == nullptr
		|| m_identify_ptr == nullptr
		|| m_control_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	//enter_dorm(1);
	//sleep(1000);
	//enter_operator_selection();
	//sleep(1000);
	//clear_and_select_the_resting();

	// for debug
	detect_mood_status_at_work(get_format_image());

	return true;
}

bool asst::InfrastDormTask::enter_dorm(int index)
{
	cv::Mat image = get_format_image();
	// 普通的和mini的，正常情况应该只有一个有结果，另一个是empty
	auto dorm_result = m_identify_ptr->find_all_images(image, "Dorm", 0.8);
	auto dorm_mini_result = m_identify_ptr->find_all_images(image, "DormMini", 0.8);

	decltype(dorm_result) cur_dorm_result;
	if (dorm_result.empty() && dorm_mini_result.empty()) {
		// 没找到宿舍，TODO，报错
		return false;
	}
	else if (dorm_result.empty()) {
		cur_dorm_result = std::move(dorm_mini_result);
	}
	else if (dorm_mini_result.empty()) {
		cur_dorm_result = std::move(dorm_result);
	}
	if (index >= cur_dorm_result.size()) {
		return false;
	}

	std::sort(cur_dorm_result.begin(), cur_dorm_result.end(), [](
		const auto& lhs, const auto& rhs) -> bool {
			return lhs.rect.y < rhs.rect.y;
		});

	m_control_ptr->click(cur_dorm_result.at(index).rect);

	return false;
}

bool asst::InfrastDormTask::enter_next_dorm()
{
	static const Rect swipe_down_begin(		// 向下滑动起点
		Configer::WindowWidthDefault * 0.3,
		Configer::WindowHeightDefault * 0.8,
		Configer::WindowWidthDefault * 0.2,
		Configer::WindowWidthDefault * 0.1);
	static const Rect swipe_down_end(		// 向下滑动终点
		Configer::WindowWidthDefault * 0.3,
		Configer::WindowHeightDefault * 0.2,
		Configer::WindowWidthDefault * 0.2,
		Configer::WindowWidthDefault * 0.1);

	static const int swipe_dwon_duration = 1000;	// 向下滑动持续时间

	m_control_ptr->swipe(swipe_down_begin, swipe_down_end, swipe_dwon_duration);

	static const Rect double_click_rect(
		Configer::WindowWidthDefault * 0.4,
		Configer::WindowHeightDefault * 0.4,
		Configer::WindowWidthDefault * 0.2,
		Configer::WindowWidthDefault * 0.2
	);

	m_control_ptr->click(double_click_rect);
	m_control_ptr->click(double_click_rect);

	return true;
}

bool asst::InfrastDormTask::enter_operator_selection()
{
	// 有这些文字之一就说明“进驻信息”这个按钮已经点开了
	static const std::vector<std::string> info_opened_flags = {
		GbkToUtf8("当前房间入住信息"),
		GbkToUtf8("进驻人数"),
		GbkToUtf8("清空")
	};

	std::vector<TextArea> ocr_result = ocr_detect();

	bool is_info_opened =
		std::find_first_of(
			ocr_result.cbegin(), ocr_result.cend(),
			info_opened_flags.cbegin(), info_opened_flags.cend(),
			[](const TextArea& lhs, const std::string& rhs)
			-> bool { return lhs.text == rhs; })
		!= ocr_result.cend();

	static const std::string station_info = GbkToUtf8("进驻信息");
	// 如果“进驻信息”窗口没点开，那就点开
	if (!is_info_opened) {
		auto station_info_iter = std::find_if(ocr_result.cbegin(), ocr_result.cend(),
			[](const TextArea& textarea) -> bool {
				return textarea.text == station_info;
			});
		m_control_ptr->click(station_info_iter->rect);
		sleep(1000);
		ocr_result = ocr_detect();
	}

	// 点击这里面任意一个，都可以进入干员选择页面
	static const std::vector<std::string> enter_operator_page_buttons = {
		GbkToUtf8("进驻"),
		GbkToUtf8("休息中"),
		GbkToUtf8("空闲中"),
		GbkToUtf8("心情")
	};

	auto button_iter = std::find_first_of(
		ocr_result.cbegin(), ocr_result.cend(),
		enter_operator_page_buttons.cbegin(), enter_operator_page_buttons.cend(),
		[](const TextArea& lhs, const std::string& rhs)
		-> bool { return lhs.text == rhs; });
	if (button_iter == ocr_result.cend()) {
		// TODO 报错
		return false;
	}
	m_control_ptr->click(button_iter->rect);

	return true;
}

bool asst::InfrastDormTask::clear_and_select_the_resting()
{
	static auto click_clear_button = [&]() {
		// 点击“清空选择”按钮
		m_control_ptr->click(Rect(430, 655, 150, 40));
		sleep(300);
	};
	cv::Mat image = get_format_image();

	std::future<void> click_clear_button_future = std::async(std::launch::async, click_clear_button);

	// 识别“休息中”的干员
	auto resting_result = m_identify_ptr->find_all_images(image, "Resting", 0.8);
	// 识别“注意力涣散”的干员
	// TODO，这个阈值太低了，不正常，有时间再调整一下模板图片
	auto listless_result = m_identify_ptr->find_all_images(image, "Listless", 0.6);
	// 识别正在工作中的干员的心情状态
	auto work_mood_result = detect_mood_status_at_work(image);

	click_clear_button_future.wait();

	int count = 0;
	// 把“休息中”的干员，都再次选上
	for (const auto& resting : resting_result) {
		m_control_ptr->click(resting.rect);
		++count;
	}
	for (const auto& listless : listless_result) {
		m_control_ptr->click(listless.rect);
		if (++count >= MaxOperNumInDorm) {
			break;
		}
	}

	return true;
}

std::vector<InfrastDormTask::MoodStatus> InfrastDormTask::detect_mood_status_at_work(const cv::Mat& image)
{
	constexpr static int MoodWidth = Configer::WindowWidthDefault * 0.0664 + 0.5;		// 心情进度条长度（满心情的时候）
	constexpr static int MoodHeight = Configer::WindowHeightDefault * 0.00416 + 0.5;	// 心情进度条高度

#ifdef LOG_TRACE
	cv::Mat draw_image = image.clone();
#endif

	// 把工作中的那个黄色笑脸全抓出来
	auto smiley_result = m_identify_ptr->find_all_images(image, "SmileyAtWork", 0.85, false);

	std::vector<MoodStatus> moods;
	for (const auto& smiley : smiley_result) {
		// 检查进度条是否超出了图片范围
		if (smiley.rect.x + smiley.rect.width + MoodWidth >= image.cols) {
			continue;
		}
		// 根据黄色笑脸往右推心情进度条的区域
		cv::Rect process_rect(
			smiley.rect.x + smiley.rect.width,
			smiley.rect.y,
			MoodWidth,
			smiley.rect.height);
		cv::Mat process_mat = image(process_rect);

		cv::Mat process_gray;
		cv::cvtColor(process_mat, process_gray, cv::COLOR_BGR2GRAY);

		int max_white_length = 0;	// 最长的横向连续的白色长条的长度
		for (int i = 0; i != process_gray.rows; ++i) {
			int cur_white_length = 0;
			cv::uint8_t left_value = 250;	// 当前点左侧的点的值
			for (int j = 0; j != process_gray.cols; ++j) {
				constexpr static cv::uint8_t LowerLimit = 180;
				constexpr static cv::uint8_t DiffThreshold = 10;

				auto value = process_gray.at<cv::uint8_t>(i, j);
				if (value >= LowerLimit && left_value - value < DiffThreshold) {	// 右边的颜色相比左边变化在阈值以内，都认为是连续的
					left_value = value;
					++cur_white_length;
					if (max_white_length < cur_white_length) {
						max_white_length = cur_white_length;
					}
				}
				else {
					if (max_white_length < cur_white_length) {
						max_white_length = cur_white_length;
					}
					left_value = 250;
					cur_white_length = 0;
				}
			}
		}

		MoodStatus mood_status;
		mood_status.process = static_cast<double>(max_white_length) / MoodWidth;
		mood_status.rect = asst::Rect(process_rect.x, process_rect.y, process_rect.width, process_rect.height);
		mood_status.actual_rect = mood_status.rect;
		mood_status.actual_rect.width = max_white_length;

#ifdef LOG_TRACE
		cv::Rect cv_actual_rect(mood_status.actual_rect.x, mood_status.actual_rect.y, 
			mood_status.actual_rect.width, mood_status.actual_rect.height);
		cv::rectangle(draw_image, cv_actual_rect, cv::Scalar(0, 0, 255), 1);
		cv::putText(draw_image, std::to_string(mood_status.process), cv::Point(cv_actual_rect.x, cv_actual_rect.y), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 0, 255));
#endif
	}

	return std::vector<MoodStatus>();
}
