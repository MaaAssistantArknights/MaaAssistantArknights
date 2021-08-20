#include "SwipeTask.h"

#include "WinMacro.h"

using namespace asst;

SwipeTask::SwipeTask(AsstCallback callback, void* callback_arg)
	: AbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeClick;
}

bool asst::SwipeTask::run()
{
	return swipe();
}

bool asst::SwipeTask::swipe()
{
	bool ret = false;
	if (!m_reverse) {
		ret = m_control_ptr->swipe(m_swipe_begin, m_swipe_end, m_swipe_duration);
	}
	else {
		ret = m_control_ptr->swipe(m_swipe_end, m_swipe_begin, m_swipe_duration);
	}
	ret &= sleep(m_swipe_duration + SwipeExtraDelay);
	return ret;
}
