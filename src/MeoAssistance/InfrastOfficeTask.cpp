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

	json::value task_start_json = json::object{
		{ "task_type",  "InfrastOfficeTask" },
		{ "task_chain", m_task_chain}
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	bool ret = swipe_right();
	ret &= enter_station({ "Office", "OfficeMini" }, 0);
	if (!ret) {
		return false;
	}
	json::value enter_json;
	enter_json["station"] = "Office";
	enter_json["index"] = 0;
	m_callback(AsstMsg::EnterStation, enter_json, m_callback_arg);

	if (enter_operator_selection()) {
		m_callback(AsstMsg::ReadyToShift, enter_json, m_callback_arg);
		select_operators(true);
		m_callback(AsstMsg::ShiftCompleted, enter_json, m_callback_arg);
	}
	else {
		m_callback(AsstMsg::NoNeedToShift, enter_json, m_callback_arg);
	}
	m_callback(AsstMsg::TaskCompleted, task_start_json, m_callback_arg);

	return true;
}

bool asst::InfrastOfficeTask::enter_operator_selection()
{
	auto suspended_result = m_identify_ptr->find_image(m_controller_ptr->get_image(), "ContactSuspended");
	if (suspended_result.score > Configer::TemplThresholdDefault) {
		m_controller_ptr->click(suspended_result.rect);
		return sleep(1000);
	}
	else {
		return false;
	}
}

int asst::InfrastOfficeTask::select_operators(bool need_to_the_left)
{
	bool ret = false;
	if (need_to_the_left) {
		ret = swipe_to_the_left();
	}
	// 办公室干员不用做识别，直接选择第一个即可
	ret &= click_first_operator();
	ret &= click_confirm_button();
	if (!ret) {
		return -1;
	}

	// 办公室只选了一个人，固定return 1
	return 1;
}