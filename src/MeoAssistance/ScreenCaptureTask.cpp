#include "ScreenCaptureTask.h"

#include "AsstAux.h"

bool asst::ScreenCaptureTask::run()
{
	if (m_controller_ptr == nullptr
		|| m_identify_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	static const std::string dirname = GetCurrentDir() + "template\\";
	return print_window(dirname, false);
}