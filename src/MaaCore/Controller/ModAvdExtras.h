#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "MaaUtils/NoWarningCVMat.hpp"
#include "Utils/LibraryHolder.hpp"

namespace asst
{
class ModAvdExtras
{
public:
    ModAvdExtras() = default;
    ~ModAvdExtras();

    bool inited() const { return inited_; }

    bool init(const std::filesystem::path& modavd_root, const std::string& adb_address);
    bool reload();
    void uninit();

    std::optional<cv::Mat> screencap();

    // 宿主侧点击（库的接口版本 2 起）：经模拟器控制台发到虚拟触屏，坐标为显示像素
    bool touch_available() const { return touch_available_; }

    std::optional<std::pair<int, int>> display_size();
    bool touch_down(int contact, int x, int y);
    bool touch_move(int contact, int x, int y);
    bool touch_up(int contact);
    bool touch_release_all();

    bool key_available() const { return key_available_; }

    bool text_available() const { return text_available_; }

    bool key_down(int android_keycode);
    bool key_up(int android_keycode);
    bool input_text(const std::string& text);

private:
    // 未启用的成员不能通过 LibraryHolder 的析构释放其他控制器持有的引用。
    using Library = LibraryHolder<ModAvdExtras>;

    // ABI: https://github.com/ModAVD/ModAVDExtras/blob/main/include/modavd_extras.h
    struct OpenParams
    {
        uint32_t size;
        const char* adb_address;
        int console_port;
        const char* client_name;
        uint32_t flags;
        int timeout_ms;
    };

    struct FrameInfo
    {
        uint32_t size;
        int32_t width;
        int32_t height;
        int32_t stride;
        uint32_t format;
        uint32_t frame_number;
        uint64_t timestamp_us;
    };

    static constexpr int kOk = 0;
    static constexpr int kErrNotReady = -3;
    static constexpr int kNotReadyRetries = 20;
    static constexpr uint32_t kCapScreencap = 1u << 0;
    static constexpr uint32_t kCapTouch = 1u << 1;
    static constexpr uint32_t kCapKey = 1u << 2;
    static constexpr uint32_t kCapText = 1u << 3;
    static constexpr uint32_t kTouchApiVersion = 2;
    static constexpr uint32_t kKeyTextApiVersion = 3;
    static constexpr uint32_t kFormatBgr = 2;

    std::optional<cv::Mat> capture_frame(int& ret);
    std::optional<cv::Mat> capture_once(int& ret);
    bool load_modavd_library();
    void unload_modavd_library();
    bool open_instance();
    void close_instance();

private:
    std::filesystem::path modavd_root_;
    std::string adb_address_;

    void* handle_ = nullptr;
    bool library_loaded_ = false;
    bool inited_ = false;
    bool touch_available_ = false;
    bool key_available_ = false;
    bool text_available_ = false;
    uint32_t api_version_ = 0;

private:
    inline static const std::string kApiVersionFuncName = "modavd_api_version";
    inline static const std::string kOpenFuncName = "modavd_open";
    inline static const std::string kCloseFuncName = "modavd_close";
    inline static const std::string kCapabilitiesFuncName = "modavd_capabilities";
    inline static const std::string kCaptureFuncName = "modavd_capture";
    inline static const std::string kStrErrorFuncName = "modavd_strerror";
    inline static const std::string kTouchDownFuncName = "modavd_touch_down";
    inline static const std::string kTouchMoveFuncName = "modavd_touch_move";
    inline static const std::string kTouchUpFuncName = "modavd_touch_up";
    inline static const std::string kTouchReleaseAllFuncName = "modavd_touch_release_all";
    inline static const std::string kKeyFuncName = "modavd_key";
    inline static const std::string kInputTextFuncName = "modavd_input_text";

    std::function<uint32_t()> api_version_func_;
    std::function<int(const OpenParams*, void**)> open_func_;
    std::function<void(void*)> close_func_;
    std::function<uint32_t(void*)> capabilities_func_;
    std::function<int(void*, int, uint32_t, uint32_t, void*, size_t, FrameInfo*)> capture_func_;
    std::function<const char*(int)> strerror_func_;
    std::function<int(void*, int, int, int, int, int)> touch_down_func_;
    std::function<int(void*, int, int, int, int, int)> touch_move_func_;
    std::function<int(void*, int, int)> touch_up_func_;
    std::function<int(void*)> touch_release_all_func_;
    std::function<int(void*, int, int, int)> key_func_;
    std::function<int(void*, const char*)> input_text_func_;
};
} // namespace asst
