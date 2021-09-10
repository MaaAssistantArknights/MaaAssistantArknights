#include "AbstractTask.h"

#include <algorithm>
#include <filesystem>
#include <thread>

#include <opencv2/opencv.hpp>

#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"
#include "AsstAux.h"

using namespace asst;

AbstractTask::AbstractTask(AsstCallback callback, void* callback_arg)
	:m_callback(callback),
	m_callback_arg(callback_arg)
{
	;
}

void AbstractTask::set_ptr(
	std::shared_ptr<WinMacro> controller_ptr,
	std::shared_ptr<Identify> identify_ptr)
{
	m_controller_ptr = controller_ptr;
	m_identify_ptr = identify_ptr;
}

void AbstractTask::set_exit_flag(bool* exit_flag)
{
	m_exit_flag = exit_flag;
}

cv::Mat AbstractTask::get_format_image(bool raw)
{
	cv::Mat raw_image = m_controller_ptr->getImage();

	if (raw_image.empty()) {
		m_callback(AsstMsg::ImageIsEmpty, json::value(), m_callback_arg);
		return raw_image;
	}

	if (raw) {
		set_control_scale(1.0);
		return raw_image;
	}
	else {
		constexpr double DefaultRatio = static_cast<double>(Configer::WindowWidthDefault) / static_cast<double>(Configer::WindowHeightDefault);
		double cur_ratio = static_cast<double>(raw_image.cols) / static_cast<double>(raw_image.rows);
		cv::Size scale_size;
		double scale = 0.0;
		if (cur_ratio >= DefaultRatio	// 说明是宽屏或默认16:9，按照高度计算缩放
			|| std::fabs(cur_ratio - DefaultRatio) < DoubleDiff)
		{
			scale_size = cv::Size(cur_ratio * Configer::WindowHeightDefault, Configer::WindowHeightDefault);
			scale = static_cast<double>(raw_image.rows) / static_cast<double>(Configer::WindowHeightDefault);
		}
		else 
		{	// 否则可能是偏正方形的屏幕，按宽度计算
			scale_size = cv::Size(Configer::WindowWidthDefault, Configer::WindowWidthDefault / cur_ratio);
			scale = static_cast<double>(raw_image.cols) / static_cast<double>(Configer::WindowWidthDefault);
		}
		cv::Mat resize_mat;
		cv::resize(raw_image, resize_mat, scale_size, cv::INPAINT_NS);
		set_control_scale(scale);

		return resize_mat;
	}
}

bool asst::AbstractTask::set_control_scale(double scale)
{
	m_controller_ptr->setControlScale(scale);
	return true;
}

//bool AbstractTask::set_control_scale(int cur_width, int cur_height)
//{
//	double scale_width = static_cast<double>(cur_width) / Configer::WindowWidthDefault;
//	double scale_height = static_cast<double>(cur_height) / Configer::WindowHeightDefault;
//	double scale = (std::max)(scale_width, scale_height);
//	m_controller_ptr->setControlScale(scale);
//	return true;
//}

bool AbstractTask::sleep(unsigned millisecond)
{
	if (need_exit()) {
		return false;
	}
	if (millisecond == 0) {
		return true;
	}
	auto start = std::chrono::system_clock::now();
	unsigned duration = 0;

	json::value callback_json;
	callback_json["time"] = millisecond;
	m_callback(AsstMsg::ReadyToSleep, callback_json, m_callback_arg);

	while (!need_exit() && duration < millisecond) {
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now() - start).count();
		std::this_thread::yield();
	}
	m_callback(AsstMsg::EndOfSleep, callback_json, m_callback_arg);

	return !need_exit();
}

bool AbstractTask::print_window(const std::string& dir)
{
	const cv::Mat& image = get_format_image(true);
	if (image.empty()) {
		return false;
	}
	// 现在用adb直接截图了，不用裁剪了
	//// 保存的截图额外再裁剪掉一圈，不然企鹅物流识别不出来
	//int offset = Configer::get_instance().m_options.print_window_crop_offset;
	//cv::Rect rect(offset, offset, image.cols - offset * 2, image.rows - offset * 2);

	std::filesystem::create_directory(dir);
	const std::string time_str = StringReplaceAll(StringReplaceAll(GetFormatTimeString(), " ", "_"), ":", "-");
	const std::string filename = dir + time_str + ".png";

	bool ret = cv::imwrite(filename.c_str(), image);

	json::value callback_json;
	callback_json["filename"] = filename;
	callback_json["ret"] = ret;
	callback_json["offset"] = 0;
	m_callback(AsstMsg::PrintWindow, callback_json, m_callback_arg);

	return ret;
}

bool asst::AbstractTask::need_exit() const noexcept
{
	return m_exit_flag != NULL && *m_exit_flag == true;
}
