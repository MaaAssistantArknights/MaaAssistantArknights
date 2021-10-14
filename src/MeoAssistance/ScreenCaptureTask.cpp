#include "ScreenCaptureTask.h"

#include "AsstUtils.hpp"

#include "Controller.h"

bool asst::ScreenCaptureTask::run()
{
    const auto& image = ctrler.get_image();

    static const std::string dirname = utils::get_cur_dir() + "template\\";
    return save_image(image, dirname);
}