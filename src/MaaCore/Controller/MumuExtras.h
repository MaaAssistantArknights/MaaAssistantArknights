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

    bool init(const std::filesystem::path& mumu_path, int mumu_inst_index);
    void set_package_name(const std::string& package_name);
    bool reload();
    void uninit();

    std::optional<cv::Mat> screencap();

private:
    bool load_mumu_library();
    bool connect_mumu();
    bool init_screencap();
    void disconnect_mumu();
    int get_display_id();

private:
    std::filesystem::path mumu_path_;
    int mumu_inst_index_ = 0;
    // mumu 的约定，default 给的是最前端 tab
    inline static std::string kDefaultPackage = "default";
    std::string package_name_ = kDefaultPackage;
    std::optional<int> display_id_cache_;

    int mumu_handle_ = 0;
    int display_width_ = 0;
    int display_height_ = 0;
    std::vector<unsigned char> display_buffer_;

    bool inited_ = false;

private:
    inline static const std::string kConnectFuncName = "nemu_connect";
    inline static const std::string kDisconnectFuncName = "nemu_disconnect";
    inline static const std::string kGetDisplayIdFuncName = "nemu_get_display_id";
    inline static const std::string kCaptureDisplayFuncName = "nemu_capture_display";
    inline static const std::string kInputTextFuncName = "nemu_input_text";
    inline static const std::string kInputEventTouchDownFuncName = "nemu_input_event_touch_down";
    inline static const std::string kInputEventTouchUpFuncName = "nemu_input_event_touch_up";
    inline static const std::string kInputEventKeyDownFuncName = "nemu_input_event_key_down";
    inline static const std::string kInputEventKeyUpFuncName = "nemu_input_event_key_up";

private:
    std::function<decltype(nemu_connect)> connect_func_;
    std::function<decltype(nemu_disconnect)> disconnect_func_;
    std::function<decltype(nemu_get_display_id)> get_display_id_func_;
    std::function<decltype(nemu_capture_display)> capture_display_func_;
    std::function<decltype(nemu_input_text)> input_text_func_;
    std::function<decltype(nemu_input_event_touch_down)> input_event_touch_down_func_;
    std::function<decltype(nemu_input_event_touch_up)> input_event_touch_up_func_;
    std::function<decltype(nemu_input_event_key_down)> input_event_key_down_func_;
    std::function<decltype(nemu_input_event_key_up)> input_event_key_up_func_;
};
}
#endif
