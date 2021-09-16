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
	//const cv::Mat& image = m_controller_ptr->get_image(true);
	//if (image.empty()) {
	//	return false;
	//}
	// 现在用adb直接截图了，不用裁剪了
	//// 保存的截图额外再裁剪掉一圈，不然企鹅物流识别不出来
	//int offset = Configer::get_instance().m_options.print_window_crop_offset;
	//cv::Rect rect(offset, offset, image.cols - offset * 2, image.rows - offset * 2);

	std::filesystem::create_directory(dir);
	const std::string time_str = StringReplaceAll(StringReplaceAll(GetFormatTimeString(), " ", "_"), ":", "-");
	const std::string filename = dir + time_str + ".png";

	bool ret = cv::imwrite(filename, m_controller_ptr->get_image(true));

	json::value callback_json;
	callback_json["filename"] = filename;
	callback_json["ret"] = ret;
	callback_json["offset"] = 0;
	m_callback(AsstMsg::PrintWindow, callback_json, m_callback_arg);

	return true;
}

bool asst::AbstractTask::need_exit() const noexcept
{
	return m_exit_flag != NULL && *m_exit_flag == true;
}

bool asst::AbstractTask::is_ptr_inited() const noexcept
{
	if (m_controller_ptr == NULL
		|| m_identify_ptr == NULL)
	{
		return false;
	}
	return true;
}