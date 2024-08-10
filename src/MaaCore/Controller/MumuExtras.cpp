#include "MumuExtras.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

namespace asst
{
MumuExtras::~MumuExtras()
{
    uninit();
}

bool MumuExtras::init(
    const std::filesystem::path& mumu_path,
    int mumu_inst_index,
    int mumu_display_id)
{
    bool same = mumu_path == mumu_path_ && mumu_inst_index == mumu_inst_index_
                && mumu_display_id == mumu_display_id_;

    if (same && inited_) {
        return true;
    }

    mumu_path_ = mumu_path;
    mumu_inst_index_ = mumu_inst_index;
    mumu_display_id_ = mumu_display_id;

    inited_ = load_mumu_library() && connect_mumu() && init_screencap();

    return inited_;
}

bool MumuExtras::reload()
{
    inited_ = load_mumu_library() && connect_mumu() && init_screencap();
    LogInfo << "Reload MumuExtras: " << VAR(inited_);
    return inited_;
}

void MumuExtras::uninit()
{
    inited_ = false;
    disconnect_mumu();
}

std::optional<cv::Mat> MumuExtras::screencap()
{
    if (!capture_display_func_) {
        LogError << "capture_display_func_ is null";
        return std::nullopt;
    }

    int ret = capture_display_func_(
        mumu_handle_,
        mumu_display_id_,
        static_cast<int>(display_buffer_.size()),
        &display_width_,
        &display_height_,
        display_buffer_.data());

    if (ret) {
        LogError << "Failed to capture display" << VAR(ret) << VAR(mumu_handle_)
                 << VAR(mumu_display_id_) << VAR(display_buffer_.size()) << VAR(display_width_)
                 << VAR(display_height_);
        return std::nullopt;
    }

    cv::Mat raw(display_height_, display_width_, CV_8UC4, display_buffer_.data());
    cv::Mat bgr;
    cv::cvtColor(raw, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat dst;
    cv::flip(bgr, dst, 0);

    return dst;
}

bool MumuExtras::load_mumu_library()
{
    auto lib_path = mumu_path_ / "shell/sdk/external_renderer_ipc";

    if (!load_library(lib_path)) {
        LogError << "Failed to load library" << VAR(lib_path);
        return false;
    }

    connect_func_ = get_function<decltype(nemu_connect)>(kConnectFuncName);
    if (!connect_func_) {
        LogError << "Failed to get function" << VAR(kConnectFuncName);
        return false;
    }

    disconnect_func_ = get_function<decltype(nemu_disconnect)>(kDisconnectFuncName);
    if (!disconnect_func_) {
        LogError << "Failed to get function" << VAR(kDisconnectFuncName);
        return false;
    }

    capture_display_func_ = get_function<decltype(nemu_capture_display)>(kCaptureDisplayFuncName);
    if (!capture_display_func_) {
        LogError << "Failed to get function" << VAR(kCaptureDisplayFuncName);
        return false;
    }

    input_text_func_ = get_function<decltype(nemu_input_text)>(kInputTextFuncName);
    if (!input_text_func_) {
        LogError << "Failed to get function" << VAR(kInputTextFuncName);
        return false;
    }

    input_event_touch_down_func_ =
        get_function<decltype(nemu_input_event_touch_down)>(kInputEventTouchDownFuncName);
    if (!input_event_touch_down_func_) {
        LogError << "Failed to get function" << VAR(kInputEventTouchDownFuncName);
        return false;
    }

    input_event_touch_up_func_ =
        get_function<decltype(nemu_input_event_touch_up)>(kInputEventTouchUpFuncName);
    if (!input_event_touch_up_func_) {
        LogError << "Failed to get function" << VAR(kInputEventTouchUpFuncName);
        return false;
    }

    input_event_key_down_func_ =
        get_function<decltype(nemu_input_event_key_down)>(kInputEventKeyDownFuncName);
    if (!input_event_key_down_func_) {
        LogError << "Failed to get function" << VAR(kInputEventKeyDownFuncName);
        return false;
    }

    input_event_key_up_func_ =
        get_function<decltype(nemu_input_event_key_up)>(kInputEventKeyUpFuncName);
    if (!input_event_key_up_func_) {
        LogError << "Failed to get function" << VAR(kInputEventKeyUpFuncName);
        return false;
    }

    return true;
}

bool MumuExtras::connect_mumu()
{
    LogInfo << VAR(mumu_path_) << VAR(mumu_inst_index_);

    if (!connect_func_) {
        LogError << "connect_func_ is null";
        return false;
    }

    std::u16string u16path = mumu_path_.u16string();
    std::wstring wpath(
        std::make_move_iterator(u16path.begin()),
        std::make_move_iterator(u16path.end()));

    mumu_handle_ = connect_func_(wpath.c_str(), mumu_inst_index_);

    if (mumu_handle_ == 0) {
        LogError << "Failed to connect mumu" << VAR(mumu_path_) << VAR(mumu_inst_index_);
        return false;
    }

    return true;
}

bool MumuExtras::init_screencap()
{
    if (!capture_display_func_) {
        LogError << "capture_display_func_ is null";
        return false;
    }

    int ret = capture_display_func_(
        mumu_handle_,
        mumu_display_id_,
        0,
        &display_width_,
        &display_height_,
        nullptr);

    // mumu 的文档给错了，这里 0 才是成功
    if (ret) {
        LogError << "Failed to capture display" << VAR(ret) << VAR(mumu_handle_)
                 << VAR(mumu_display_id_);
        return false;
    }

    display_buffer_.resize(display_width_ * display_height_ * 4);

    LogDebug << VAR(display_width_) << VAR(display_height_) << VAR(display_buffer_.size());
    return true;
}

void MumuExtras::disconnect_mumu()
{
    LogInfo << VAR(mumu_handle_);

    if (mumu_handle_ != 0) {
        disconnect_func_(mumu_handle_);
    }
}

}
#endif
