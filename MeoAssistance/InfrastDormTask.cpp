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
	
	enter_upper_dorm();

	return true;
}

bool asst::InfrastDormTask::enter_upper_dorm()
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
	// 最顶上的宿舍（第一个宿舍）
	auto upper_iter = std::min_element(cur_dorm_result.cbegin(), cur_dorm_result.cend(), [](
		const auto& lhs, const auto& rhs) -> bool {
			return lhs.rect.y < rhs.rect.y;
		});
	if (upper_iter == cur_dorm_result.cend()) {
		// 按理说走不到这里，TODO 报错
		return false;
	}
	m_control_ptr->click(upper_iter->rect);

	return false;
}
