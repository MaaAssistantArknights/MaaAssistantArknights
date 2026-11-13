// 结构参照 MumuExtras.cpp
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
// ModAVD 截图增强：载入 ModAVD 自带的 modavd_extras 动态库（<ModAVD 根目录>/extras/），
// 读模拟器的截图共享内存。与 MumuExtras / LDExtras 不同，它不依赖 3rdparty/EmulatorExtras，
// 不受 ASST_WITH_EMULATOR_EXTRAS 控制，Windows / Linux / macOS 都可用。
// 库的接口见 https://github.com/ModAVD/ModAVDExtras/blob/main/include/modavd_extras.h
// 不继承 LibraryHolder：它的析构无条件归还一次引用，而引用计数按类型全进程共用。AdbController
// 总会构造这个成员，没载过库的实例析构时，会把同一进程里别的控制器正在用的库卸掉。
// 这里只用它的静态函数，自己记住取没取到引用，取到了才还。
class ModAvdExtras
{
public:
    ModAvdExtras() = default;
    ~ModAvdExtras();

    bool inited() const { return inited_; }

    // modavd_root 为 ModAVD 根目录；adb_address 为连接地址（127.0.0.1:<adb 端口> 或 emulator-<控制台端口>），
    // 由库推出实例的控制台端口
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

private:
    using Library = LibraryHolder<ModAvdExtras>;

    // 与 modavd_extras.h 的同名结构体逐字段一致；size 字段让新旧版本的库互相兼容
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

    // modavd_extras.h 里的常量
    static constexpr int kOk = 0;
    static constexpr uint32_t kCapScreencap = 1u << 0;
    static constexpr uint32_t kCapTouch = 1u << 1;
    static constexpr uint32_t kTouchApiVersion = 2;
    static constexpr uint32_t kFormatBgr = 2;

    // 截一帧；ret 为库最后一次返回的出错码，kOk 表示失败与库无关
    std::optional<cv::Mat> capture_frame(int& ret);
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
};
} // namespace asst
