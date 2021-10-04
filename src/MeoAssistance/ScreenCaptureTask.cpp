#include "ScreenCaptureTask.h"

#include "AsstAux.h"

#include "WinMacro.h"
#include "Identify.h"

bool asst::ScreenCaptureTask::run()
{
    if (m_controller_ptr == nullptr
        || m_identify_ptr == nullptr)
    {
        m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
        return false;
    }

    auto image = m_controller_ptr->get_image();
    auto res = m_identify_ptr->penguin_recognize(image);

    static const std::string dirname = GetCurrentDir() + "template\\";
    return print_window(dirname, false);
}