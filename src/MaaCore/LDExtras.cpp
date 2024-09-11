#include "LDExtras.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/Platform/SafeWindows.h"
#include <string>
#include <sstream>

namespace asst
{
LDExtras::~LDExtras()
{
    uninit();
}

bool LDExtras::init(HWND hwnd, unsigned int ld_inst_index, const std::filesystem::path& ld_path)
{
    bool same = hwnd == hwnd_ && ld_inst_index == ld_inst_index_;

    if (same && inited_) {
        return true;
    }

    hwnd_ = hwnd;
    ld_inst_index_ = ld_inst_index;
    ld_path_ = ld_path;

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

std::optional<cv::Mat> LDExtras::screencap()
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

    cv::Mat raw(display_height_, display_width_, CV_8UC4, pixels);
    cv::Mat bgr;
    cv::cvtColor(raw, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat dst;
    cv::flip(bgr, dst, 0);

    screenshot_instance_->release();

    return dst;
}

bool LDExtras::load_ld_library()
{
    auto lib_path = ld_path_ / "ldopengl64.dll";

    if (!load_library(lib_path)) {
        LogError << "Failed to load library" << VAR(lib_path);
        return false;
    }

    initial_gl_func_ = get_function<decltype(initialGL)>(kInitialGLFuncName);
    if (!initial_gl_func_) {
        LogError << "Failed to get function" << VAR(kInitialGLFuncName);
        return false;
    }

    uninitial_gl_func_ = get_function<decltype(uninitialGL)>(kUninitialGLFuncName);
    if (!uninitial_gl_func_) {
        LogError << "Failed to get function" << VAR(kUninitialGLFuncName);
        return false;
    }

    read_pixels_func_ = get_function<decltype(readPixels)>(kReadPixelsFuncName);
    if (!read_pixels_func_) {
        LogError << "Failed to get function" << VAR(kReadPixelsFuncName);
        return false;
    }

    create_screenshot_instance_func_ = get_function<decltype(CreateScreenShotInstance)>(kCreateScreenShotInstanceFuncName);
    if (!create_screenshot_instance_func_) {
        LogError << "Failed to get function" << VAR(kCreateScreenShotInstanceFuncName);
        return false;
    }

    return true;
}

bool LDExtras::connect_ld()
{
    LogInfo << VAR(hwnd_) << VAR(ld_inst_index_);

    if (!initial_gl_func_) {
        LogError << "initial_gl_func_ is null";
        return false;
    }

    if (!initial_gl_func_(hwnd_, ld_inst_index_, nullptr)) {
        LogError << "Failed to initialize GL" << VAR(hwnd_) << VAR(ld_inst_index_);
        return false;
    }

    screenshot_instance_ = create_screenshot_instance_func_(ld_inst_index_, 0);
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

    // 假设我们有一个方法来获取显示宽度和高度
    display_width_ = 1280; // 请根据实际情况修改
    display_height_ = 720; // 请根据实际情况修改

    display_buffer_.resize(display_width_ * display_height_ * 4);

    LogDebug << VAR(display_width_) << VAR(display_height_) << VAR(display_buffer_.size());
    return true;
}

void LDExtras::disconnect_ld()
{
    LogInfo << "Disconnecting LD";

    if (screenshot_instance_) {
        screenshot_instance_->release();
        screenshot_instance_ = nullptr;
    }

    if (uninitial_gl_func_) {
        uninitial_gl_func_();
    }
}

}
#endif
