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
	std::shared_ptr<WinMacro> window_ptr,
	std::shared_ptr<WinMacro> view_ptr,
	std::shared_ptr<WinMacro> control_ptr,
	std::shared_ptr<Identify> identify_ptr)
{
	m_window_ptr = window_ptr;
	m_view_ptr = view_ptr;
	m_control_ptr = control_ptr;
	m_identify_ptr = identify_ptr;
}

void AbstractTask::set_exit_flag(bool* exit_flag)
{
	m_exit_flag = exit_flag;
}

cv::Mat AbstractTask::get_format_image(bool hd)
{
	cv::Mat raw_image;
	if (hd) {
		raw_image = m_view_ptr->getAdbImage();
	}
	else {
		raw_image = m_view_ptr->getImage(m_view_ptr->getWindowRect());
	}
	if (raw_image.empty()) {
		m_callback(AsstMsg::ImageIsEmpty, json::value(), m_callback_arg);
		return raw_image;
	}
	if (raw_image.rows < 100) {
		m_callback(AsstMsg::WindowMinimized, json::value(), m_callback_arg);
		return raw_image;
	}

	if (hd) {
		set_control_scale(1.0);
		return raw_image;
	}
	else {
		// 把模拟器边框的一圈裁剪掉
		const EmulatorInfo& window_info = m_view_ptr->getEmulatorInfo();
		int x_offset = window_info.x_offset;
		int y_offset = window_info.y_offset;
		int width = raw_image.cols - x_offset - window_info.right_offset;
		int height = raw_image.rows - y_offset - window_info.bottom_offset;

		cv::Mat cropped(raw_image, cv::Rect(x_offset, y_offset, width, height));

		// 根据图像尺寸，调整控制的缩放
		set_control_scale(cropped.cols, cropped.rows);

		//// 调整尺寸，与资源中截图的标准尺寸一致
		//cv::Mat dst;
		//cv::resize(cropped, dst, cv::Size(m_configer.DefaultWindowWidth, m_configer.DefaultWindowHeight));

		return cropped;
	}
}

bool asst::AbstractTask::set_control_scale(double scale)
{
	m_control_ptr->setControlScale(scale, true);
	return true;
}

bool AbstractTask::set_control_scale(int cur_width, int cur_height)
{
	double scale_width = static_cast<double>(cur_width) / Configer::DefaultWindowWidth;
	double scale_height = static_cast<double>(cur_height) / Configer::DefaultWindowHeight;
	double scale = (std::max)(scale_width, scale_height);
	m_control_ptr->setControlScale(scale);
	return true;
}

bool AbstractTask::sleep(unsigned millisecond)
{
	if (millisecond == 0) {
		return true;
	}
	auto start = std::chrono::system_clock::now();
	unsigned duration = 0;

	json::value callback_json;
	callback_json["time"] = millisecond;
	m_callback(AsstMsg::ReadyToSleep, callback_json, m_callback_arg);

	while ((m_exit_flag == NULL || *m_exit_flag == false)
		&& duration < millisecond) {
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now() - start).count();
		std::this_thread::yield();
	}
	m_callback(AsstMsg::EndOfSleep, callback_json, m_callback_arg);

	return (m_exit_flag == NULL || *m_exit_flag == false);
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