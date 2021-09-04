#include "InfrastDormTask.h"

#include <opencv2/opencv.hpp>

#include "WinMacro.h"
#include "Identify.h"

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
	
	enter_dorm(2);

	return true;
}

bool asst::InfrastDormTask::enter_dorm(int index)
{
	cv::Mat image = get_format_image();
	// 普通的和mini的，正常情况应该只有一个有结果，另一个是empty
	// 为了防止识别漏了，这里阈值先放低一点
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
