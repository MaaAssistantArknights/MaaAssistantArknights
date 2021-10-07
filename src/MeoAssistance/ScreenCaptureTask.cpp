#include "ScreenCaptureTask.h"

#include "AsstAux.h"

#include "Controller.h"

bool asst::ScreenCaptureTask::run()
{
    const auto& image = ctrler.get_image();

    static const std::string dirname = GetCurrentDir() + "template\\";
    return save_image(image, dirname);
}