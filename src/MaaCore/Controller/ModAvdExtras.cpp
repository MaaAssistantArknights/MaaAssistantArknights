#include "ModAvdExtras.h"

#include <chrono>
#include <thread>

#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

namespace asst
{
ModAvdExtras::~ModAvdExtras()
{
    uninit();
    unload_modavd_library();
}

bool ModAvdExtras::init(const std::filesystem::path& modavd_root, const std::string& adb_address)
{
    if (inited_ && modavd_root == modavd_root_ && adb_address == adb_address_) {
        return true;
    }

    close_instance();
    // LibraryHolder 按类型共享库，切换根目录前需释放旧库。
    if (library_loaded_ && modavd_root != modavd_root_) {
        unload_modavd_library();
    }
    modavd_root_ = modavd_root;
    adb_address_ = adb_address;

    inited_ = (library_loaded_ || load_modavd_library()) && open_instance();
    return inited_;
}

bool ModAvdExtras::reload()
{
    close_instance();
    unload_modavd_library();

    inited_ = load_modavd_library() && open_instance();
    LogInfo << "Reload ModAvdExtras: " << VAR(inited_);
    return inited_;
}

void ModAvdExtras::uninit()
{
    inited_ = false;
    close_instance();
}

std::optional<cv::Mat> ModAvdExtras::screencap()
{
    try {
        if (!handle_ || !capture_func_) {
            LogError << "ModAvdExtras is not opened";
            return std::nullopt;
        }

        int ret = kOk;
        auto image = capture_frame(ret);
        if (image || ret == kOk) {
            return image;
        }

        if (!reload()) {
            LogError << "ModAvdExtras capture failed and reload failed" << VAR(ret);
            return std::nullopt;
        }
        image = capture_frame(ret);
        if (!image) {
            LogError << "ModAvdExtras capture failed even after reload" << VAR(ret);
        }
        return image;
    }
    catch (const cv::Exception& e) {
        if (e.code == cv::Error::StsNoMem) {
            throw;
        }
        try {
            LogError << "ModAvdExtras screencap OpenCV exception:" << e.what() << VAR(e.code) << VAR(e.file)
                     << VAR(e.line);
        }
        catch (...) {
        }
        return std::nullopt;
    }
}

std::optional<cv::Mat> ModAvdExtras::capture_frame(int& ret)
{
    auto image = capture_once(ret);
    // 首帧未就绪或复制期间帧持续更新时，直接重试，避免重开共享句柄抬起正在按住的手指。
    for (int i = 0; !image && ret == kErrNotReady && i < kNotReadyRetries; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        image = capture_once(ret);
    }
    return image;
}

std::optional<cv::Mat> ModAvdExtras::capture_once(int& ret)
{
    FrameInfo info {};
    info.size = sizeof(info);
    ret = capture_func_(handle_, 0, kFormatBgr, 0, nullptr, 0, &info);
    if (ret != kOk || info.width <= 0 || info.height <= 0) {
        LogWarn << "ModAvdExtras size probe failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "")
                << VAR(info.width) << VAR(info.height);
        return std::nullopt;
    }

    cv::Mat image(info.height, info.width, CV_8UC3);
    if (static_cast<size_t>(info.stride) != image.step[0]) {
        LogError << "ModAvdExtras unexpected stride" << VAR(info.stride) << VAR(image.step[0]);
        return std::nullopt;
    }

    ret = capture_func_(handle_, 0, kFormatBgr, 0, image.data, image.total() * image.elemSize(), &info);
    if (ret != kOk) {
        LogWarn << "ModAvdExtras capture failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "");
        return std::nullopt;
    }

    // 两次调用之间可能旋转或缩小画面；缓冲区仍够用时，库会成功返回新的尺寸。
    if (info.width != image.cols || info.height != image.rows) {
        return cv::Mat(info.height, info.width, CV_8UC3, image.data, info.stride).clone();
    }

    return image;
}

bool ModAvdExtras::load_modavd_library()
{
    using namespace asst::utils::path_literals;

    auto lib_path = modavd_root_ / "extras"_p / "modavd_extras"_p;
    if (!Library::load_library(lib_path)) {
        LogError << "Failed to load ModAVD extras library" << VAR(lib_path);
        unload_modavd_library();
        return false;
    }
    library_loaded_ = true;
    LogInfo << "Successfully loaded ModAVD extras library from: " << lib_path;

    api_version_func_ = Library::get_function<uint32_t()>(kApiVersionFuncName);
    open_func_ = Library::get_function<int(const OpenParams*, void**)>(kOpenFuncName);
    close_func_ = Library::get_function<void(void*)>(kCloseFuncName);
    capabilities_func_ = Library::get_function<uint32_t(void*)>(kCapabilitiesFuncName);
    capture_func_ =
        Library::get_function<int(void*, int, uint32_t, uint32_t, void*, size_t, FrameInfo*)>(kCaptureFuncName);
    strerror_func_ = Library::get_function<const char*(int)>(kStrErrorFuncName);
    // 点击的四个函数接口版本 1 起就导出，版本 2 起才能用；可选，缺了只是不用宿主侧点击
    touch_down_func_ = Library::get_function<int(void*, int, int, int, int, int)>(kTouchDownFuncName);
    touch_move_func_ = Library::get_function<int(void*, int, int, int, int, int)>(kTouchMoveFuncName);
    touch_up_func_ = Library::get_function<int(void*, int, int)>(kTouchUpFuncName);
    touch_release_all_func_ = Library::get_function<int(void*)>(kTouchReleaseAllFuncName);
    // 按键、文字同样版本 1 起就导出，版本 3 起才能用
    key_func_ = Library::get_function<int(void*, int, int, int)>(kKeyFuncName);
    input_text_func_ = Library::get_function<int(void*, const char*)>(kInputTextFuncName);

    if (!api_version_func_ || !open_func_ || !close_func_ || !capabilities_func_ || !capture_func_) {
        LogError << "ModAVD extras library is missing functions";
        unload_modavd_library();
        return false;
    }

    uint32_t api_version = api_version_func_();
    api_version_ = api_version;
    LogInfo << "ModAVD extras" << VAR(api_version);
    if (api_version < 1) {
        LogError << "ModAVD extras library is too old" << VAR(api_version);
        unload_modavd_library();
        return false;
    }
    return true;
}

void ModAvdExtras::unload_modavd_library()
{
    api_version_func_ = nullptr;
    open_func_ = nullptr;
    close_func_ = nullptr;
    capabilities_func_ = nullptr;
    capture_func_ = nullptr;
    strerror_func_ = nullptr;
    touch_down_func_ = nullptr;
    touch_move_func_ = nullptr;
    touch_up_func_ = nullptr;
    touch_release_all_func_ = nullptr;
    key_func_ = nullptr;
    input_text_func_ = nullptr;
    touch_available_ = false;
    key_available_ = false;
    text_available_ = false;
    api_version_ = 0;

    if (library_loaded_) {
        Library::unload_library();
        library_loaded_ = false;
    }
}

bool ModAvdExtras::open_instance()
{
    if (!open_func_ || !capabilities_func_ || !close_func_) {
        return false;
    }

    OpenParams params {};
    params.size = sizeof(params);
    params.adb_address = adb_address_.c_str();
    params.client_name = "MaaCore";

    void* handle = nullptr;
    int ret = open_func_(&params, &handle);
    if (ret != kOk || !handle) {
        LogError << "Failed to open ModAVD instance" << VAR(adb_address_) << VAR(ret)
                 << VAR(strerror_func_ ? strerror_func_(ret) : "");
        return false;
    }

    uint32_t caps = capabilities_func_(handle);
    if (!(caps & kCapScreencap)) {
        LogError << "ModAVD instance cannot capture" << VAR(adb_address_);
        close_func_(handle);
        return false;
    }

    handle_ = handle;
    touch_available_ = api_version_ >= kTouchApiVersion && (caps & kCapTouch) && touch_down_func_ && touch_move_func_ &&
                       touch_up_func_ && touch_release_all_func_;
    key_available_ = api_version_ >= kKeyTextApiVersion && (caps & kCapKey) && key_func_;
    text_available_ = api_version_ >= kKeyTextApiVersion && (caps & kCapText) && input_text_func_;
    LogInfo << "Opened ModAVD instance" << VAR(adb_address_) << VAR(touch_available_) << VAR(key_available_)
            << VAR(text_available_);
    return true;
}

void ModAvdExtras::close_instance()
{
    if (handle_ && close_func_) {
        close_func_(handle_);
    }
    handle_ = nullptr;
    touch_available_ = false;
    key_available_ = false;
    text_available_ = false;
}

std::optional<std::pair<int, int>> ModAvdExtras::display_size()
{
    if (!handle_ || !capture_func_) {
        return std::nullopt;
    }
    FrameInfo info {};
    info.size = sizeof(info);
    int ret = capture_func_(handle_, 0, kFormatBgr, 0, nullptr, 0, &info);
    if (ret != kOk || info.width <= 0 || info.height <= 0) {
        return std::nullopt;
    }
    return std::make_pair(info.width, info.height);
}

bool ModAvdExtras::touch_down(int contact, int x, int y)
{
    if (!touch_available_ || !handle_) {
        return false;
    }
    int ret = touch_down_func_(handle_, 0, contact, x, y, 0);
    if (ret != kOk) {
        LogWarn << "ModAvdExtras touch_down failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "")
                << VAR(contact) << VAR(x) << VAR(y);
    }
    return ret == kOk;
}

bool ModAvdExtras::touch_move(int contact, int x, int y)
{
    if (!touch_available_ || !handle_) {
        return false;
    }
    int ret = touch_move_func_(handle_, 0, contact, x, y, 0);
    if (ret != kOk) {
        LogWarn << "ModAvdExtras touch_move failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "")
                << VAR(contact) << VAR(x) << VAR(y);
    }
    return ret == kOk;
}

bool ModAvdExtras::touch_up(int contact)
{
    if (!touch_available_ || !handle_) {
        return false;
    }
    int ret = touch_up_func_(handle_, 0, contact);
    if (ret != kOk) {
        LogWarn << "ModAvdExtras touch_up failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "")
                << VAR(contact);
    }
    return ret == kOk;
}

bool ModAvdExtras::touch_release_all()
{
    if (!touch_available_ || !handle_) {
        return false;
    }
    int ret = touch_release_all_func_(handle_);
    if (ret != kOk) {
        LogWarn << "ModAvdExtras touch_release_all failed" << VAR(ret)
                << VAR(strerror_func_ ? strerror_func_(ret) : "");
    }
    return ret == kOk;
}

bool ModAvdExtras::key_down(int android_keycode)
{
    if (!key_available_ || !handle_) {
        return false;
    }
    int ret = key_func_(handle_, 0, android_keycode, 1);
    if (ret != kOk) {
        LogWarn << "ModAvdExtras key_down failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "")
                << VAR(android_keycode);
    }
    return ret == kOk;
}

bool ModAvdExtras::key_up(int android_keycode)
{
    if (!key_available_ || !handle_) {
        return false;
    }
    int ret = key_func_(handle_, 0, android_keycode, 0);
    if (ret != kOk) {
        LogWarn << "ModAvdExtras key_up failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "")
                << VAR(android_keycode);
    }
    return ret == kOk;
}

bool ModAvdExtras::input_text(const std::string& text)
{
    if (!text_available_ || !handle_) {
        return false;
    }
    int ret = input_text_func_(handle_, text.c_str());
    if (ret != kOk) {
        // 没有可交文字的编辑框而文字又不是 ASCII 时库返回不支持，什么都没打出去
        LogWarn << "ModAvdExtras input_text failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "");
    }
    return ret == kOk;
}
} // namespace asst
