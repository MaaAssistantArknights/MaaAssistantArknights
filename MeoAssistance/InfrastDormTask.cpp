#include "InfrastDormTask.h"

#include <future>
#include <thread>

#include <opencv2/opencv.hpp>

#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"

using namespace asst;

bool asst::InfrastDormTask::run()
{
	if (m_controller_ptr == nullptr
		|| m_identify_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	// for debug 
	//m_select_with_swipe = true;

	enter_station({"Dorm", "DormMini"}, m_dorm_begin, 0.8);

	int dorm_index = m_dorm_begin;
	for (; dorm_index < DormNum; ++dorm_index) {
		bool to_left = false;
		if (dorm_index != m_dorm_begin) {
			enter_next_dorm();
			to_left = false;
		}
		else {
			to_left = true;	// 第一个进入的宿舍需要滑动到最左侧一下。后面的宿舍都不用了
		}
		enter_operator_selection();
		int selected = 0;
		if (m_select_with_swipe) {
			selected = select_operators_with_swipe(to_left);
		}
		else {
			selected = select_operators(to_left);
		}
		if (selected < MaxOperNumInDorm) {	// 如果选不满5个人，说明没有更多需要休息的了，直接结束宿舍任务
			break;
		}
	}
	m_dorm_begin = dorm_index;

	return true;
}

bool asst::InfrastDormTask::click_confirm_button()
{
	InfrastAbstractTask::click_confirm_button();
	sleep(300);

	// 点完确定后，如果把工作中的干员撤下来了，会再弹出来一个确认的界面，如果没撤下来则不会弹出。先识别一下再决定要不要点击
	auto&& [algorithm, score, second_confirm_rect] =
		m_identify_ptr->find_image(m_controller_ptr->get_image(), "DormConfirm");
	if (score >= Configer::TemplThresholdDefault) {
		m_controller_ptr->click(second_confirm_rect);
	}
	sleep(2000);
	return true;
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

	// 游戏bug，宿舍中如果“进驻信息”已被选中，直接进行滑动会被滑的很远
	// 所以这里先检查一下，如果进驻信息被选中了，就先把它关了，再进行滑动
	auto find_result = m_identify_ptr->find_image(m_controller_ptr->get_image(), "StationInfoSelected");
	if (find_result.score >= 0.75) {
		m_controller_ptr->click(find_result.rect);
	}

	m_controller_ptr->swipe(swipe_down_begin, swipe_down_end, swipe_dwon_duration);

	static const Rect double_click_rect(
		Configer::WindowWidthDefault * 0.4,
		Configer::WindowHeightDefault * 0.4,
		Configer::WindowWidthDefault * 0.2,
		Configer::WindowWidthDefault * 0.2
	);

	// 游戏中的宿舍里，双击可以让当前设施回到正确的位置
	m_controller_ptr->click(double_click_rect);
	m_controller_ptr->click(double_click_rect);

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
		m_controller_ptr->click(station_info_iter->rect);
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
	m_controller_ptr->click(button_iter->rect);
	sleep(3000);

	return true;
}

int asst::InfrastDormTask::select_operators(const cv::Mat& image)
{
	// 识别“休息中”的干员
	auto resting_result = m_identify_ptr->find_all_images(image, "Resting", 0.8);
	if (resting_result.size() == MaxOperNumInDorm) {	// 如果所有人都在休息，那这个宿舍不用换班，直接关了
		return MaxOperNumInDorm;
	}

	// 识别“注意力涣散”的干员
	// TODO，这个阈值太低了，不正常，有时间再调整一下模板图片
	auto listless_result = m_identify_ptr->find_all_images(image, "Listless", 0.6);
	// 识别正在工作中的干员的心情状态
	auto work_mood_result = detect_mood_status_at_work(image, Configer::get_instance().m_infrast_options.dorm_threshold);

	int count = 0;
	// 把“休息中”的干员，都再次选上
	for (const auto& resting : resting_result) {
		m_controller_ptr->click(resting.rect);
		++count;
	}

	if (listless_result.size() == 0 && work_mood_result.size() == 0) {	// 如果没有注意力涣散的和心情低的，也直接关了
		return 0;
	}

	// 选择“注意力涣散”的干员
	for (const auto& listless : listless_result) {
		if (count++ >= MaxOperNumInDorm) {
			break;
		}
		m_controller_ptr->click(listless.rect);
	}
	// 选择心情较低的干员
	for (const auto& mood_status : work_mood_result) {
		if (count++ >= MaxOperNumInDorm) {
			break;
		}
		m_controller_ptr->click(mood_status.rect);
	}
	return count;
}

int asst::InfrastDormTask::select_operators(bool need_to_the_left)
{
	if (need_to_the_left) {
		swipe_to_the_left();
	}

	cv::Mat image = m_controller_ptr->get_image();
	click_clear_button();
	int count = select_operators(image);
	if (count == 0) {
		click_return_button();
	}
	else {
		click_confirm_button();
	}

	return count;
}

int asst::InfrastDormTask::select_operators_with_swipe(bool need_to_the_left)
{
	if (need_to_the_left) {
		swipe_to_the_left();
	}
	cv::Mat image = m_controller_ptr->get_image();
	click_clear_button();

	int count = 0;
	while (true) {
		count += select_operators(image);

		if (count >= MaxOperNumInDorm) {
			break;
		}
			 
		auto rest_result = m_identify_ptr->find_image(image, "SmileyAtRest");
		if (rest_result.score > Configer::TemplThresholdDefault) {
			break;
		}
		swipe();
		image = m_controller_ptr->get_image();
	}

	if (count == 0) {
		click_return_button();
	}
	else {
		click_confirm_button();
	}

	return count;
}

std::vector<InfrastDormTask::MoodStatus> InfrastDormTask::detect_mood_status_at_work(const cv::Mat& image, double process_threshold) const
{
	constexpr static int MoodWidth = Configer::WindowWidthDefault * 0.0664 + 0.5;		// 心情进度条长度（满心情的时候）
	constexpr static int MoodHeight = Configer::WindowHeightDefault * 0.00416 + 0.5;	// 心情进度条高度

#ifdef LOG_TRACE
	cv::Mat draw_image = image.clone();
#endif

	// 把工作中的那个黄色笑脸全抓出来
	auto smiley_result = m_identify_ptr->find_all_images(image, "SmileyAtWork", 0.8, false);

	std::vector<MoodStatus> moods_vec;
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
				constexpr static cv::uint8_t DiffThreshold = 20;

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
		mood_status.actual_length = max_white_length;
		mood_status.process = static_cast<double>(max_white_length) / MoodWidth;
		mood_status.rect = asst::Rect(smiley.rect.x, smiley.rect.y, 
			smiley.rect.width + process_rect.width, smiley.rect.height + process_rect.height);
		mood_status.actual_rect = asst::Rect(process_rect.x, process_rect.y, 
			max_white_length, process_rect.height);
#ifdef LOG_TRACE
		cv::Rect cv_actual_rect(mood_status.actual_rect.x, mood_status.actual_rect.y,
			mood_status.actual_rect.width, mood_status.actual_rect.height);
		cv::rectangle(draw_image, cv_actual_rect, cv::Scalar(0, 0, 255), 1);
		cv::putText(draw_image, std::to_string(mood_status.process), cv::Point(cv_actual_rect.x, cv_actual_rect.y), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 0, 255));
#endif
		if (mood_status.process == 0) {
			// 值为0说明是“注意力涣散”，红色哭脸被错误的识别成黄色笑脸了，这里直接忽略这个值
		}
		else if (mood_status.process <= process_threshold) {	// 心情小于阈值的直接加入结果
			moods_vec.emplace_back(std::move(mood_status));
		}
		else {
			// 既不在工作中，也不在休息中，但是心情值还大于阈值的，也需要加入结果中
			// 思路：根据心情进度条的位置，往上裁剪出“工作中”or“休息中”的那一块区域
			// 进行模板匹配，如果没匹配上，则认为这个干员既不在工作，也不在休息
			constexpr static int HeightDiff = Configer::WindowHeightDefault * 0.13;
			constexpr static int OnShiftHeight = Configer::WindowHeightDefault * 0.08;
			cv::Rect on_shift_rect(mood_status.rect.x, mood_status.rect.y - HeightDiff, mood_status.rect.width, OnShiftHeight);
			// “休息中”的笑脸前面的模板匹配是对不上的，走不到这里，所以这里只匹配“工作中”即可
			auto find_result = m_identify_ptr->find_image(image(on_shift_rect), "OnShift");
			if (find_result.score < 0.5) {
				moods_vec.emplace_back(std::move(mood_status));
			}
		}
	}

	std::sort(moods_vec.begin(), moods_vec.end(),
		[](const auto& lhs, const auto& rhs) -> bool {
			// 按剩余心情进度排个序，少的在前面
			if (lhs.actual_length != rhs.actual_length) {
				return lhs.actual_length < rhs.actual_length;
			}
			// 如果剩余心情相等，优先选靠左侧、靠上侧的，更符合用户直觉
			else if (lhs.rect.x != rhs.rect.x) {
				return lhs.rect.x < rhs.rect.x;
			}
			else {
				return lhs.rect.y < rhs.rect.y;
			}
		});

	return moods_vec;
}