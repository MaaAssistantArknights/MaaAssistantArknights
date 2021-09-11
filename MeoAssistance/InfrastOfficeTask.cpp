#include "InfrastOfficeTask.h"

#include "Configer.h"
#include "Identify.h"
#include "WinMacro.h"

bool asst::InfrastOfficeTask::run()
{
	if (m_controller_ptr == nullptr
		|| m_identify_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	swipe_right();
	enter_station({ "Office", "OfficeMin" }, 0);
	if (enter_operator_selection()) {
		select_operators(true);
	}
	return true;
}

bool asst::InfrastOfficeTask::enter_operator_selection()
{
	auto suspended_result = m_identify_ptr->find_image(m_controller_ptr->get_image(), "ContactSuspended");
	if (suspended_result.score > Configer::TemplThresholdDefault) {
		m_controller_ptr->click(suspended_result.rect);
		sleep(1000);
		return true;
	}
	else {
		return false;
	}
}

int asst::InfrastOfficeTask::select_operators(bool need_to_the_left)
{
	if (need_to_the_left) {
		swipe_to_the_left();
	}
	// 办公室干员不用做识别，直接选择第一个即可
	click_first_operator();
	click_confirm_button();

	return 1;
}
