#include "LDExtras.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/Platform/SafeWindows.h"
#include <Controller/AdbController.h>
#include <sstream>
#include <string>

namespace asst
{
LDExtras::~LDExtras()
{
    uninit();
}

bool LDExtras::init(
    const std::filesystem::path& ld_path,
    unsigned int ld_inst_index,
    unsigned int ld_pid,
    int width,
    int height)
{
    const bool same = ld_inst_index == ld_inst_index_ && ld_pid == ld_pid_;

    if (same && inited_) {
        return true;
    }

    ld_inst_index_ = ld_inst_index;
    ld_path_ = ld_path;
    ld_pid_ = ld_pid;
    display_width_ = width;
    display_height_ = height;

    inited_ = load_ld_library() && connect_ld() && init_screencap();

    return inited_;
}

bool LDExtras::reload()
{
    inited_ = load_ld_library() && connect_ld() && init_screencap();
    LogInfo << "Reload LDExtras: " << VAR(inited_);
    return inited_;
}

void LDExtras::uninit()
{
    inited_ = false;
    disconnect_ld();
}

std::optional<cv::Mat> LDExtras::screencap() const
{
    if (!screenshot_instance_) {
        LogError << "screenshot_instance_ is null";
        return std::nullopt;
    }

    void* pixels = screenshot_instance_->cap();
    if (!pixels) {
        LogError << "Failed to capture screen";
        return std::nullopt;
    }

    const cv::Mat raw(display_height_, display_width_, CV_8UC3, pixels);
    cv::Mat dst;
    cv::flip(raw, dst, 0);

    return dst;
}

bool LDExtras::load_ld_library()
{
    auto lib_path = ld_path_ / "ldopengl64.dll";

    if (!load_library(lib_path)) {
        LogError << "Failed to load library" << VAR(lib_path);
        return false;
    }

    create_screenshot_instance_func_ =
        get_function<decltype(CreateScreenShotInstance)>(kCreateScreenShotInstanceFuncName);
    if (!create_screenshot_instance_func_) {
        LogError << "Failed to get function" << VAR(kCreateScreenShotInstanceFuncName);
        return false;
    }

    return true;
}

bool LDExtras::connect_ld()
{
    LogInfo << VAR(ld_inst_index_) << VAR(ld_pid_);

    screenshot_instance_ = create_screenshot_instance_func_(ld_inst_index_, ld_pid_);
    if (!screenshot_instance_) {
        LogError << "Failed to create screenshot instance" << VAR(ld_inst_index_);
        return false;
    }

    return true;
}

bool LDExtras::init_screencap()
{
    if (!screenshot_instance_) {
        LogError << "screenshot_instance_ is null";
        return false;
    }

    LogDebug << VAR(display_width_) << VAR(display_height_);
    return true;
}

void LDExtras::disconnect_ld()
{
    LogInfo << "Disconnecting LD";

    if (screenshot_instance_) {
        screenshot_instance_->release();
        screenshot_instance_ = nullptr;
    }
}

}
#endif
