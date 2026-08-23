#include "MumuExtras.h"

#if ASST_WITH_EMULATOR_EXTRAS

#include <unordered_map>

#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/StringMisc.hpp"

namespace asst
{
MumuExtras::~MumuExtras()
{
    uninit();
}

bool MumuExtras::init(const std::filesystem::path& mumu_path, int mumu_inst_index, bool enable_input)
{
    bool same = mumu_path == mumu_path_ && mumu_inst_index == mumu_inst_index_ && enable_input == input_enabled_;

    if (same && inited_) {
        return true;
    }

    mumu_path_ = mumu_path;
    mumu_inst_index_ = mumu_inst_index;
    input_enabled_ = enable_input;
    // 换了模拟器路径或开关，之前的版本探测结果不作数了
    input_version_checked_ = false;

    inited_ = load_mumu_library() && connect_mumu() && init_screencap();

    return inited_;
}

void MumuExtras::set_package_name(const std::string& package_name)
{
    if (package_name.empty()) {
        package_name_ = kDefaultPackage;
    }
    else {
        package_name_ = package_name;
    }

    // 换了包名，之前缓存的 display id 就不作数了
    invalidate_display_id();
}

bool MumuExtras::reload()
{
    invalidate_display_id();

    // LibraryHolder::load_library 对已加载的库只是 ++ref_count_，
    // 不先释放的话每次 reload 都会多留一份引用，dll 永远不会被卸载
    if (library_loaded_) {
        disconnect_mumu();
        unload_library();
        library_loaded_ = false;
    }

    inited_ = load_mumu_library() && connect_mumu() && init_screencap();
    LogInfo << "Reload MumuExtras: " << VAR(inited_) << VAR(input_available_);
    return inited_;
}

void MumuExtras::uninit()
{
    inited_ = false;
    input_available_ = false;
    invalidate_display_id();
    disconnect_mumu();
}

std::optional<cv::Mat> MumuExtras::screencap()
{
    try {
        if (!capture_display_func_) {
            LogError << "capture_display_func_ is null";
            return std::nullopt;
        }

        auto display_id = get_display_id();
        if (!display_id) {
            LogError << "Failed to get display id";
            return std::nullopt;
        }

        int ret = capture_display_func_(
            mumu_handle_,
            *display_id,
            static_cast<int>(display_buffer_.size()),
            &display_width_,
            &display_height_,
            display_buffer_.data());

        if (ret) {
            // Try reloading once before giving up.
            if (!reload()) {
                LogError << "Failed to capture display and failed to reload. " << VAR(ret) << VAR(mumu_handle_)
                         << VAR(*display_id) << VAR(display_buffer_.size()) << VAR(display_width_) << VAR(display_height_);
                return std::nullopt;
            }
            // Reload 之后 display_id 可能变化，重新获取后再重试一次 capture。
            display_id = get_display_id();
            if (!display_id) {
                LogError << "Failed to get display id after reload";
                return std::nullopt;
            }
            ret = capture_display_func_(
                mumu_handle_,
                *display_id,
                static_cast<int>(display_buffer_.size()),
                &display_width_,
                &display_height_,
                display_buffer_.data());
            if (ret) {
                LogError << "Failed to capture display even after reload. " << VAR(ret) << VAR(mumu_handle_)
                         << VAR(*display_id) << VAR(display_buffer_.size()) << VAR(display_width_) << VAR(display_height_);
                return std::nullopt;
            }
        }

        cv::Mat raw(display_height_, display_width_, CV_8UC4, display_buffer_.data());
        cv::Mat bgr;
        cv::cvtColor(raw, bgr, cv::COLOR_RGBA2BGR);
        cv::Mat dst;
        cv::flip(bgr, dst, 0);

        return dst;
    }
    catch (const cv::Exception& e) {
        if (e.code == cv::Error::StsNoMem) {
            throw;
        }
        try {
            LogError << "MumuExtras screencap OpenCV exception:" << e.what() << VAR(e.code) << VAR(e.file)
                     << VAR(e.line);
        }
        catch (...) {
        }
        return std::nullopt;
    }
}

bool MumuExtras::touch_down(int contact, int x, int y)
{
    if (!input_event_finger_touch_down_func_) {
        LogError << "input_event_finger_touch_down_func_ is null";
        return false;
    }

    auto display_id = get_display_id();
    if (!display_id) {
        LogError << "Failed to get display id, skip touch_down" << VAR(contact) << VAR(x) << VAR(y);
        return false;
    }

    // contact 从 0 开始，mumu 的 finger_id 从 1 开始
    int ret = input_event_finger_touch_down_func_(mumu_handle_, *display_id, contact + 1, x, y);
    if (ret) {
        LogError << "Failed to touch_down" << VAR(ret) << VAR(contact) << VAR(x) << VAR(y) << VAR(*display_id);
        return false;
    }

    return true;
}

bool MumuExtras::touch_move(int contact, int x, int y)
{
    // mumu 没有单独的 move，重复 touch_down 即可
    return touch_down(contact, x, y);
}

bool MumuExtras::touch_up(int contact)
{
    if (!input_event_finger_touch_up_func_) {
        LogError << "input_event_finger_touch_up_func_ is null";
        return false;
    }

    auto display_id = get_display_id();
    if (!display_id) {
        LogError << "Failed to get display id, skip touch_up" << VAR(contact);
        return false;
    }

    int ret = input_event_finger_touch_up_func_(mumu_handle_, *display_id, contact + 1);
    if (ret) {
        LogError << "Failed to touch_up" << VAR(ret) << VAR(contact) << VAR(*display_id);
        return false;
    }

    return true;
}

bool MumuExtras::key_down(int android_keycode)
{
    if (!input_event_key_down_func_) {
        LogError << "input_event_key_down_func_ is null";
        return false;
    }

    auto key_code = android_keycode_to_linux_key_code(android_keycode);
    if (!key_code) {
        return false;
    }

    auto display_id = get_display_id();
    if (!display_id) {
        LogError << "Failed to get display id, skip key_down" << VAR(android_keycode);
        return false;
    }

    int ret = input_event_key_down_func_(mumu_handle_, *display_id, *key_code);
    if (ret) {
        LogError << "Failed to key_down" << VAR(ret) << VAR(android_keycode) << VAR(*key_code) << VAR(*display_id);
        return false;
    }

    return true;
}

bool MumuExtras::key_up(int android_keycode)
{
    if (!input_event_key_up_func_) {
        LogError << "input_event_key_up_func_ is null";
        return false;
    }

    auto key_code = android_keycode_to_linux_key_code(android_keycode);
    if (!key_code) {
        return false;
    }

    auto display_id = get_display_id();
    if (!display_id) {
        LogError << "Failed to get display id, skip key_up" << VAR(android_keycode);
        return false;
    }

    int ret = input_event_key_up_func_(mumu_handle_, *display_id, *key_code);
    if (ret) {
        LogError << "Failed to key_up" << VAR(ret) << VAR(android_keycode) << VAR(*key_code) << VAR(*display_id);
        return false;
    }

    return true;
}

bool MumuExtras::input_text(const std::string& text)
{
    if (!input_text_func_) {
        LogError << "input_text_func_ is null";
        return false;
    }

    // 只记长度：输入内容可能含账号密码等敏感信息，不该进日志
    LogDebug << VAR(text.size());

    int ret = input_text_func_(mumu_handle_, static_cast<int>(text.size()), text.c_str());
    if (ret) {
        LogError << "Failed to input_text" << VAR(ret);
        return false;
    }

    return true;
}

bool MumuExtras::load_mumu_library()
{
    // 候选路径列表，按版本从新到旧排列，新增版本只需追加一项
    static const std::vector<std::filesystem::path> kCandidateRelativePaths = {
        "nx_device/15.0/shell/sdk/external_renderer_ipc", // MuMu 6.0
        "nx_device/12.0/shell/sdk/external_renderer_ipc", // MuMu 5.0 / MuMu 12
        "shell/sdk/external_renderer_ipc",                // MuMu 旧版本
    };

    bool loaded = false;
    for (const auto& rel_path : kCandidateRelativePaths) {
        auto lib_path = mumu_path_ / rel_path;
        if (load_library(lib_path)) {
            LogInfo << "Successfully loaded MuMu external renderer library from: " << lib_path;
            loaded = true;
            library_loaded_ = true;
            break;
        }
    }

    if (!loaded) {
        LogError << "Failed to load library from all candidate paths";
        for (const auto& rel_path : kCandidateRelativePaths) {
            LogError << "  tried: " << (mumu_path_ / rel_path);
        }
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

    get_display_id_func_ = get_function<decltype(nemu_get_display_id)>(kGetDisplayIdFuncName);
    if (!get_display_id_func_) {
        // 旧版本 mumu 没这个函数, 兼容一下
        LogWarn << "Failed to get function, please update your MuMu Player" << VAR(kGetDisplayIdFuncName);
        // return false;
    }

    capture_display_func_ = get_function<decltype(nemu_capture_display)>(kCaptureDisplayFuncName);
    if (!capture_display_func_) {
        LogError << "Failed to get function" << VAR(kCaptureDisplayFuncName);
        return false;
    }

    // 输入是可选能力，缺失时只影响触控，不能连累截图。
    // 版本探测要起子进程，reload() 时不该重复付这份开销，所以结果缓存下来。
    if (!input_version_checked_) {
        input_version_ok_ = input_enabled_ && check_input_version();
        input_version_checked_ = true;
    }
    input_available_ = input_enabled_ && input_version_ok_ && load_input_functions();
    LogInfo << "MuMu extras input" << VAR(input_enabled_) << VAR(input_available_);

    return true;
}

bool MumuExtras::load_input_functions()
{
    input_text_func_ = get_function<decltype(nemu_input_text)>(kInputTextFuncName);
    if (!input_text_func_) {
        LogWarn << "Failed to get function" << VAR(kInputTextFuncName);
        return false;
    }

    input_event_finger_touch_down_func_ =
        get_function<decltype(nemu_input_event_finger_touch_down)>(kInputEventFingerTouchDownFuncName);
    if (!input_event_finger_touch_down_func_) {
        LogWarn << "Failed to get function" << VAR(kInputEventFingerTouchDownFuncName);
        return false;
    }

    input_event_finger_touch_up_func_ =
        get_function<decltype(nemu_input_event_finger_touch_up)>(kInputEventFingerTouchUpFuncName);
    if (!input_event_finger_touch_up_func_) {
        LogWarn << "Failed to get function" << VAR(kInputEventFingerTouchUpFuncName);
        return false;
    }

    input_event_key_down_func_ = get_function<decltype(nemu_input_event_key_down)>(kInputEventKeyDownFuncName);
    if (!input_event_key_down_func_) {
        LogWarn << "Failed to get function" << VAR(kInputEventKeyDownFuncName);
        return false;
    }

    input_event_key_up_func_ = get_function<decltype(nemu_input_event_key_up)>(kInputEventKeyUpFuncName);
    if (!input_event_key_up_func_) {
        LogWarn << "Failed to get function" << VAR(kInputEventKeyUpFuncName);
        return false;
    }

    return true;
}

bool MumuExtras::check_input_version() const
{
    // MuMuManager.exe 与 adb 同目录，按版本从新到旧排列
    static const std::vector<std::filesystem::path> kCandidateRelativePaths = {
        "nx_main/MuMuManager.exe", // MuMu v5+
        "shell/MuMuManager.exe",   // MuMu v4 及更早
    };

    std::filesystem::path mgr_path;
    for (const auto& rel_path : kCandidateRelativePaths) {
        auto full_path = mumu_path_ / rel_path;
        if (std::filesystem::exists(full_path)) {
            mgr_path = full_path;
            break;
        }
    }

    if (mgr_path.empty()) {
        LogWarn << "MuMuManager not found, disable extras input" << VAR(mumu_path_);
        return false;
    }

    // 与 MaaFramework 一致：在 MuMuManager 所在目录启动，并设超时，避免 call_command 卡死拖垮连接
    std::string cmd = "\"" + utils::path_to_utf8_string(mgr_path) + "\" version";
    constexpr int kVersionTimeoutMs = 5000;
    std::string output = utils::call_command(cmd, nullptr, mgr_path.parent_path(), kVersionTimeoutMs);
    LogDebug << VAR(cmd) << VAR(output);

    // { "version": "6.3.2.0" }
    auto jopt = json::parse(output);
    if (!jopt || !jopt->is_object()) {
        LogWarn << "Parse MuMuManager version failed" << VAR(output);
        return false;
    }

    std::string version = jopt->get("version", "");
    if (version.empty()) {
        LogWarn << "MuMuManager version field is empty" << VAR(output);
        return false;
    }

    // 低于 6.3.2.0 的 external renderer 输入行为异常，只启用截图
    static constexpr int kMinVersion[] = { 6, 3, 2, 0 };
    std::string_view remain = version;
    for (int expected : kMinVersion) {
        std::string_view part = remain;
        if (auto pos = remain.find('.'); pos != std::string_view::npos) {
            part = remain.substr(0, pos);
            remain = remain.substr(pos + 1);
        }
        else {
            remain = {};
        }

        // 版本号位数不足时按 0 补齐，如 "6.4" 视作 6.4.0.0
        int value = 0;
        if (!part.empty() && !utils::chars_to_number<int, true>(part, value)) {
            LogWarn << "Invalid MuMuManager version" << VAR(version);
            return false;
        }

        if (value != expected) {
            bool supported = value > expected;
            LogInfo << "MuMuManager version" << VAR(version) << VAR(supported);
            return supported;
        }
    }

    LogInfo << "MuMuManager version" << VAR(version) << ", supported";
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
    std::wstring wpath(std::make_move_iterator(u16path.begin()), std::make_move_iterator(u16path.end()));

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

    auto display_id = get_display_id();
    if (!display_id) {
        LogError << "Failed to get display id";
        return false;
    }
    LogInfo << "Get display id" << VAR(*display_id);

    int ret = capture_display_func_(mumu_handle_, *display_id, 0, &display_width_, &display_height_, nullptr);

    // mumu 的文档给错了，这里 0 才是成功
    if (ret) {
        LogError << "Failed to capture display" << VAR(ret) << VAR(mumu_handle_) << VAR(*display_id);
        return false;
    }

    display_buffer_.resize(static_cast<size_t>(display_width_) * display_height_ * 4);

    LogDebug << VAR(display_width_) << VAR(display_height_) << VAR(display_buffer_.size());
    return true;
}

void MumuExtras::disconnect_mumu()
{
    LogInfo << VAR(mumu_handle_);

    if (mumu_handle_ != 0 && disconnect_func_) {
        disconnect_func_(mumu_handle_);
        // 清零，避免 reload 路径下拿旧 handle 重复 disconnect
        mumu_handle_ = 0;
    }
}

std::optional<int> MumuExtras::get_display_id()
{
    // swipe 的 move 每几毫秒一次，每次都问一遍 dll 太贵了
    int cached = display_id_cache_.load(std::memory_order_relaxed);
    if (cached != kInvalidDisplayId) {
        return cached;
    }

    if (!get_display_id_func_) {
        // 旧版本 mumu 没这个函数，此时只有 display 0，按 0 处理
        LogWarn << "get_display_id_func_ is null, fallback to 0, please update your MuMu Player";
        display_id_cache_.store(0, std::memory_order_relaxed);
        return 0;
    }

    // 优先用真实包名（明日方舟），失败再退到 mumu 约定的 "default"（最前端 tab），
    // 都失败则对齐 MaaFramework 退回 0（主 display）
    // 只有真实包名命中时才缓存：连接时游戏可能还没开，包名查不到只能拿到桌面 display，
    // 如果把 fallback 结果也缓存了，等游戏启动后截图/触控会一直锁在错误的 display 上
    int id = get_display_id_func_(mumu_handle_, package_name_.c_str(), 0);
    if (id >= 0) {
        LogInfo << "MuMu display id" << VAR(id) << VAR(package_name_);
        display_id_cache_.store(id, std::memory_order_relaxed);
        return id;
    }

    // fallback 路径：不缓存，下次会重新尝试真实包名
    LogWarn << "Failed to get display id for package, try default" << VAR(id) << VAR(package_name_);
    if (package_name_ != kDefaultPackage) {
        id = get_display_id_func_(mumu_handle_, kDefaultPackage.c_str(), 0);
    }
    if (id < 0) {
        LogWarn << "Failed to get display id, fallback to 0" << VAR(id) << VAR(package_name_);
        id = 0;
    }

    LogInfo << "MuMu display id (not cached)" << VAR(id) << VAR(package_name_);
    return id;
}

std::optional<int> MumuExtras::android_keycode_to_linux_key_code(int key)
{
    // https://developer.android.com/reference/android/view/KeyEvent
    // https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
    static const std::unordered_map<int, int> kMap = {
        // Letters
        { 29, 30 }, // KEYCODE_A -> KEY_A
        { 30, 48 }, // KEYCODE_B -> KEY_B
        { 31, 46 }, // KEYCODE_C -> KEY_C
        { 32, 32 }, // KEYCODE_D -> KEY_D
        { 33, 18 }, // KEYCODE_E -> KEY_E
        { 34, 33 }, // KEYCODE_F -> KEY_F
        { 35, 34 }, // KEYCODE_G -> KEY_G
        { 36, 35 }, // KEYCODE_H -> KEY_H
        { 37, 23 }, // KEYCODE_I -> KEY_I
        { 38, 36 }, // KEYCODE_J -> KEY_J
        { 39, 37 }, // KEYCODE_K -> KEY_K
        { 40, 38 }, // KEYCODE_L -> KEY_L
        { 41, 50 }, // KEYCODE_M -> KEY_M
        { 42, 49 }, // KEYCODE_N -> KEY_N
        { 43, 24 }, // KEYCODE_O -> KEY_O
        { 44, 25 }, // KEYCODE_P -> KEY_P
        { 45, 16 }, // KEYCODE_Q -> KEY_Q
        { 46, 19 }, // KEYCODE_R -> KEY_R
        { 47, 31 }, // KEYCODE_S -> KEY_S
        { 48, 20 }, // KEYCODE_T -> KEY_T
        { 49, 22 }, // KEYCODE_U -> KEY_U
        { 50, 47 }, // KEYCODE_V -> KEY_V
        { 51, 17 }, // KEYCODE_W -> KEY_W
        { 52, 45 }, // KEYCODE_X -> KEY_X
        { 53, 21 }, // KEYCODE_Y -> KEY_Y
        { 54, 44 }, // KEYCODE_Z -> KEY_Z

        // Numbers (top row)
        { 7, 11 },  // KEYCODE_0 -> KEY_0
        { 8, 2 },   // KEYCODE_1 -> KEY_1
        { 9, 3 },   // KEYCODE_2 -> KEY_2
        { 10, 4 },  // KEYCODE_3 -> KEY_3
        { 11, 5 },  // KEYCODE_4 -> KEY_4
        { 12, 6 },  // KEYCODE_5 -> KEY_5
        { 13, 7 },  // KEYCODE_6 -> KEY_6
        { 14, 8 },  // KEYCODE_7 -> KEY_7
        { 15, 9 },  // KEYCODE_8 -> KEY_8
        { 16, 10 }, // KEYCODE_9 -> KEY_9

        // Function keys
        { 131, 59 }, // KEYCODE_F1 -> KEY_F1
        { 132, 60 }, // KEYCODE_F2 -> KEY_F2
        { 133, 61 }, // KEYCODE_F3 -> KEY_F3
        { 134, 62 }, // KEYCODE_F4 -> KEY_F4
        { 135, 63 }, // KEYCODE_F5 -> KEY_F5
        { 136, 64 }, // KEYCODE_F6 -> KEY_F6
        { 137, 65 }, // KEYCODE_F7 -> KEY_F7
        { 138, 66 }, // KEYCODE_F8 -> KEY_F8
        { 139, 67 }, // KEYCODE_F9 -> KEY_F9
        { 140, 68 }, // KEYCODE_F10 -> KEY_F10
        { 141, 87 }, // KEYCODE_F11 -> KEY_F11
        { 142, 88 }, // KEYCODE_F12 -> KEY_F12

        // Navigation
        { 19, 103 }, // KEYCODE_DPAD_UP -> KEY_UP
        { 20, 108 }, // KEYCODE_DPAD_DOWN -> KEY_DOWN
        { 21, 105 }, // KEYCODE_DPAD_LEFT -> KEY_LEFT
        { 22, 106 }, // KEYCODE_DPAD_RIGHT -> KEY_RIGHT
        { 23, 28 },  // KEYCODE_DPAD_CENTER -> KEY_ENTER

        // Space, Enter, Backspace, Tab, Escape
        { 62, 57 }, // KEYCODE_SPACE -> KEY_SPACE
        { 66, 28 }, // KEYCODE_ENTER -> KEY_ENTER
        { 67, 14 }, // KEYCODE_DEL -> KEY_BACKSPACE
        { 61, 15 }, // KEYCODE_TAB -> KEY_TAB
        { 111, 1 }, // KEYCODE_ESCAPE -> KEY_ESC

        // Shift, Ctrl, Alt, CapsLock, Meta
        { 59, 42 },   // KEYCODE_SHIFT_LEFT -> KEY_LEFTSHIFT
        { 60, 54 },   // KEYCODE_SHIFT_RIGHT -> KEY_RIGHTSHIFT
        { 113, 29 },  // KEYCODE_CTRL_LEFT -> KEY_LEFTCTRL
        { 114, 97 },  // KEYCODE_CTRL_RIGHT -> KEY_RIGHTCTRL
        { 57, 56 },   // KEYCODE_ALT_LEFT -> KEY_LEFTALT
        { 58, 100 },  // KEYCODE_ALT_RIGHT -> KEY_RIGHTALT
        { 115, 58 },  // KEYCODE_CAPS_LOCK -> KEY_CAPSLOCK
        { 117, 125 }, // KEYCODE_META_LEFT -> KEY_LEFTMETA
        { 118, 126 }, // KEYCODE_META_RIGHT -> KEY_RIGHTMETA

        // Symbols
        { 76, 53 },   // KEYCODE_SLASH -> KEY_SLASH
        { 122, 102 }, // KEYCODE_MOVE_HOME -> KEY_HOME
        { 123, 107 }, // KEYCODE_MOVE_END -> KEY_END
        { 124, 110 }, // KEYCODE_INSERT -> KEY_INSERT
        { 92, 104 },  // KEYCODE_PAGE_UP -> KEY_PAGEUP
        { 93, 109 },  // KEYCODE_PAGE_DOWN -> KEY_PAGEDOWN
        { 112, 111 }, // KEYCODE_FORWARD_DEL -> KEY_DELETE

        // Misc
        { 4, 158 },   // KEYCODE_BACK -> KEY_BACK
        { 3, 102 },   // KEYCODE_HOME -> KEY_HOME
        { 82, 139 },  // KEYCODE_MENU -> KEY_MENU
        { 84, 217 },  // KEYCODE_SEARCH -> KEY_SEARCH
        { 85, 164 },  // KEYCODE_MEDIA_PLAY_PAUSE -> KEY_PLAYPAUSE
        { 86, 166 },  // KEYCODE_MEDIA_STOP -> KEY_STOPCD
        { 87, 163 },  // KEYCODE_MEDIA_NEXT -> KEY_NEXTSONG
        { 88, 165 },  // KEYCODE_MEDIA_PREVIOUS -> KEY_PREVIOUSSONG
        { 89, 168 },  // KEYCODE_MEDIA_REWIND -> KEY_REWIND
        { 90, 208 },  // KEYCODE_MEDIA_FAST_FORWARD -> KEY_FASTFORWARD
        { 24, 115 },  // KEYCODE_VOLUME_UP -> KEY_VOLUMEUP
        { 25, 114 },  // KEYCODE_VOLUME_DOWN -> KEY_VOLUMEDOWN
        { 164, 113 }, // KEYCODE_VOLUME_MUTE -> KEY_MUTE
    };

    auto it = kMap.find(key);
    if (it == kMap.end()) {
        // 两套编码互不相干，原样透传几乎必然按错键，不如不按
        LogError << "unknown android key, no linux keycode mapping" << VAR(key);
        return std::nullopt;
    }
    return it->second;
}
} // namespace asst

#endif
