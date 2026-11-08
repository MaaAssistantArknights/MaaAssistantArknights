// 结构参照 MumuExtras.cpp
#include "ModAvdExtras.h"

#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

namespace asst
{
ModAvdExtras::~ModAvdExtras()
{
    uninit();
    // 只还自己取到的那份引用
    unload_modavd_library();
}

bool ModAvdExtras::init(const std::filesystem::path& modavd_root, const std::string& adb_address)
{
    if (inited_ && modavd_root == modavd_root_ && adb_address == adb_address_) {
        return true;
    }

    close_instance();
    // LibraryHolder 一个类型只持有一份库，换了根目录要先还掉旧的，才能从新目录载入
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
    // LibraryHolder::load_library 对已加载的库只会 ++ref_count_，先还再取，避免引用泄漏
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
            // 截到了，或者失败的原因不是库报错（重开也没用）
            return image;
        }

        // 库报错（尺寸变了的 MODAVD_E_RESIZED、拷贝途中一直有新帧写入的 MODAVD_E_NOT_READY 等）：
        // 和 MumuExtras 一样先自己重开一次再整套重做，免得一次可恢复的错误就让调用方重新测速
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
    // 先问尺寸，再直接拷进 BGR 的 Mat：库交的就是 BGR、上下正向，不需要再转换
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

    return image;
}

bool ModAvdExtras::load_modavd_library()
{
    using namespace asst::utils::path_literals;

    // LibraryHolder 自己补平台的前后缀：extras/modavd_extras.dll、extras/libmodavd_extras.so / .dylib
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

    if (!api_version_func_ || !open_func_ || !close_func_ || !capabilities_func_ || !capture_func_) {
        LogError << "ModAVD extras library is missing functions";
        unload_modavd_library();
        return false;
    }

    // 接口版本只增不减，1 起就有截图
    uint32_t api_version = api_version_func_();
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
    // 函数对象指向库里的代码，库卸了就不能再留着
    api_version_func_ = nullptr;
    open_func_ = nullptr;
    close_func_ = nullptr;
    capabilities_func_ = nullptr;
    capture_func_ = nullptr;
    strerror_func_ = nullptr;

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

    if (!(capabilities_func_(handle) & kCapScreencap)) {
        LogError << "ModAVD instance cannot capture" << VAR(adb_address_);
        close_func_(handle);
        return false;
    }

    handle_ = handle;
    LogInfo << "Opened ModAVD instance" << VAR(adb_address_);
    return true;
}

void ModAvdExtras::close_instance()
{
    if (handle_ && close_func_) {
        close_func_(handle_);
    }
    handle_ = nullptr;
}
} // namespace asst
