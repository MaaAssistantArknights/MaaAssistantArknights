#pragma once

#include "Common/AsstConf.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include <filesystem>
#include <optional>
#include <string>

#include "Mumu/external_renderer_ipc/external_renderer_ipc.h"

#include "Utils/LibraryHolder.hpp"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
class MumuExtras : public LibraryHolder<MumuExtras>
{
public:
    MumuExtras() = default;

    virtual ~MumuExtras();

    bool inited() const { return inited_; }


    bool init(const std::filesystem::path& mumu_path, int mumu_inst_index, int mumu_display_id);
    void uninit();

    std::optional<cv::Mat> screencap();

private:
    bool load_mumu_library();
    bool connect_mumu();
    bool init_screencap();
    void disconnect_mumu();

private:
    std::filesystem::path mumu_path_;
    int mumu_inst_index_ = 0;
    int mumu_display_id_ = 0;

    int mumu_handle_ = 0;
    int display_width_ = 0;
    int display_height_ = 0;
    std::vector<unsigned char> display_buffer_;

    bool inited_ = false;

private:
    inline static const std::string kConnectFuncName = "nemu_connect";
    inline static const std::string kDisconnectFuncName = "nemu_disconnect";
    inline static const std::string kCaptureDisplayFuncName = "nemu_capture_display";
    inline static const std::string kInputTextFuncName = "nemu_input_text";
    inline static const std::string kInputEventTouchDownFuncName = "nemu_input_event_touch_down";
    inline static const std::string kInputEventTouchUpFuncName = "nemu_input_event_touch_up";
    inline static const std::string kInputEventKeyDownFuncName = "nemu_input_event_key_down";
    inline static const std::string kInputEventKeyUpFuncName = "nemu_input_event_key_up";

private:
    std::function<decltype(nemu_connect)> connect_func_;
    std::function<decltype(nemu_disconnect)> disconnect_func_;
    std::function<decltype(nemu_capture_display)> capture_display_func_;
    std::function<decltype(nemu_input_text)> input_text_func_;
    std::function<decltype(nemu_input_event_touch_down)> input_event_touch_down_func_;
    std::function<decltype(nemu_input_event_touch_up)> input_event_touch_up_func_;
    std::function<decltype(nemu_input_event_key_down)> input_event_key_down_func_;
    std::function<decltype(nemu_input_event_key_up)> input_event_key_up_func_;
};
}
#endif
