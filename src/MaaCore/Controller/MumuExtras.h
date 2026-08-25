#pragma once

#if ASST_WITH_EMULATOR_EXTRAS

#include <atomic>
#include <filesystem>
#include <optional>
#include <string>

#include "Mumu/external_renderer_ipc/external_renderer_ipc.h"

#include "MaaUtils/NoWarningCVMat.hpp"
#include "Utils/LibraryHolder.hpp"

namespace asst
{
class MumuExtras : public LibraryHolder<MumuExtras>
{
public:
    MumuExtras() = default;

    virtual ~MumuExtras();

    bool inited() const { return inited_; }

    // 触控是否可用：需要 dll 导出了 input 相关符号，且 MuMuManager 版本达标
    bool input_available() const { return inited_ && input_available_; }

    // enable_input 为 false 时只初始化截图，跳过 input 符号解析和 MuMuManager 版本探测
    bool init(const std::filesystem::path& mumu_path, int mumu_inst_index, bool enable_input);
    void set_package_name(const std::string& package_name);
    bool reload();
    void uninit();

    std::optional<cv::Mat> screencap();

    std::pair<int, int> get_display_size() const { return { display_width_, display_height_ }; }

    // 当前 display 是否为包名命中的缓存结果（而非 fallback）。fallback 时游戏未在渲染，
    // 拿到的 display 及其尺寸都与目标游戏无关，不能用于判断触控坐标系是否一致
    bool has_target_display() const { return display_id_cache_.load(std::memory_order_relaxed) != kInvalidDisplayId; }

    // contact 为 0 起始，内部转换为 mumu 的 1 起始 finger_id
    bool touch_down(int contact, int x, int y);
    bool touch_move(int contact, int x, int y);
    bool touch_up(int contact);

    // keycode 为 Android KeyEvent 值，内部转换为 Linux input-event-code
    bool key_down(int android_keycode);
    bool key_up(int android_keycode);

    bool input_text(const std::string& text);

private:
    bool load_mumu_library();
    bool load_input_functions();
    bool connect_mumu();
    bool init_screencap();
    void disconnect_mumu();
    // 0 是合法的 display id，查询失败必须和它区分开，否则会把输入打到错误的 display
    std::optional<int> get_display_id();

    void invalidate_display_id() { display_id_cache_ = kInvalidDisplayId; }

    // MuMuManager.exe version >= 6.3.2.0 才支持 external renderer 输入，低版本行为异常
    bool check_input_version() const;

    // 无对应映射时返回 nullopt：两套编码互不相干，透传等于按错键
    static std::optional<int> android_keycode_to_linux_key_code(int key);

private:
    std::filesystem::path mumu_path_;
    int mumu_inst_index_ = 0;
    // mumu 的约定，default 给的是最前端 tab
    inline static std::string kDefaultPackage = "default";
    std::string package_name_ = kDefaultPackage;

    int mumu_handle_ = 0;
    int display_width_ = 0;
    int display_height_ = 0;
    std::vector<unsigned char> display_buffer_;

    // swipe 的 move 每几毫秒一次，不缓存 display_id 会把 dll 调用打满
    static constexpr int kInvalidDisplayId = -1;
    std::atomic<int> display_id_cache_ = kInvalidDisplayId;

    // 本对象是否持有一份 LibraryHolder 引用，reload 时用来先还再取，避免引用计数泄漏
    bool library_loaded_ = false;

    bool inited_ = false;
    bool input_enabled_ = false;   // 上层是否请求了触控
    bool input_available_ = false; // 触控是否真的可用（符号齐全 + 版本达标）
    // 版本探测要起子进程，reload() 时不重复执行
    bool input_version_checked_ = false;
    bool input_version_ok_ = false;

private:
    inline static const std::string kConnectFuncName = "nemu_connect";
    inline static const std::string kDisconnectFuncName = "nemu_disconnect";
    inline static const std::string kGetDisplayIdFuncName = "nemu_get_display_id";
    inline static const std::string kCaptureDisplayFuncName = "nemu_capture_display";
    inline static const std::string kInputTextFuncName = "nemu_input_text";
    inline static const std::string kInputEventFingerTouchDownFuncName = "nemu_input_event_finger_touch_down";
    inline static const std::string kInputEventFingerTouchUpFuncName = "nemu_input_event_finger_touch_up";
    inline static const std::string kInputEventKeyDownFuncName = "nemu_input_event_key_down";
    inline static const std::string kInputEventKeyUpFuncName = "nemu_input_event_key_up";

private:
    std::function<decltype(nemu_connect)> connect_func_;
    std::function<decltype(nemu_disconnect)> disconnect_func_;
    std::function<decltype(nemu_get_display_id)> get_display_id_func_;
    std::function<decltype(nemu_capture_display)> capture_display_func_;
    std::function<decltype(nemu_input_text)> input_text_func_;
    std::function<decltype(nemu_input_event_finger_touch_down)> input_event_finger_touch_down_func_;
    std::function<decltype(nemu_input_event_finger_touch_up)> input_event_finger_touch_up_func_;
    std::function<decltype(nemu_input_event_key_down)> input_event_key_down_func_;
    std::function<decltype(nemu_input_event_key_up)> input_event_key_up_func_;
};
}
#endif
