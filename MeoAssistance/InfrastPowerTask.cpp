#include "InfrastPowerTask.h"

#include "WinMacro.h"
#include "Identify.h"

bool asst::InfrastPowerTask::run()
{
	if (m_controller_ptr == nullptr
		|| m_identify_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	for (int i = 0; i != PowerNum; ++i) {
		enter_station({ "Power", "PowerMini"}, i);
		sleep(1000);
		click_return_button();
		sleep(1000);
	}
    return true;
}