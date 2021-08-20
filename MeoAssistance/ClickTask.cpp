#include "ClickTask.h"

#include "WinMacro.h"

using namespace asst;

ClickTask::ClickTask(AsstCallback callback, void* callback_arg)
	: AbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeClick;
}

bool ClickTask::run()
{
	if (m_control_ptr == NULL)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}
	json::value task_start_json = json::object{
		{ "task_type",  "ClickTask" },
		{ "task_chain", m_task_chain},
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	m_control_ptr->click(m_rect);
	return true;
}