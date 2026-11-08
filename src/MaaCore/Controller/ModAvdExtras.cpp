// 结构参照 MumuExtras.cpp
#include "ModAvdExtras.h"

#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"

namespace asst
{
ModAvdExtras::~ModAvdExtras()
{
    // 库的引用由 ~LibraryHolder 归还，和 MumuExtras 一样
    uninit();
}

bool ModAvdExtras::init(const std::filesystem::path& modavd_root, const std::string& adb_address)
{
    if (inited_ && modavd_root == modavd_root_ && adb_address == adb_address_) {
        return true;
    }

    close_instance();
    modavd_root_ = modavd_root;
    adb_address_ = adb_address;

    inited_ = (library_loaded_ || load_modavd_library()) && open_instance();
    return inited_;
}

bool ModAvdExtras::reload()
{
    // LibraryHolder::load_library 对已加载的库只会 ++ref_count_，先还再取，避免引用泄漏
    close_instance();
    if (library_loaded_) {
        unload_library();
        library_loaded_ = false;
    }

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
    if (!handle_ || !capture_func_) {
        LogError << "ModAvdExtras is not opened";
        return std::nullopt;
    }

    // 先问尺寸，再直接拷进 BGR 的 Mat：库交的就是 BGR、上下正向，不需要再转换
    FrameInfo info {};
    info.size = sizeof(info);
    int ret = capture_func_(handle_, 0, kFormatBgr, 0, nullptr, 0, &info);
    if (ret != kOk || info.width <= 0 || info.height <= 0) {
        LogError << "ModAvdExtras size probe failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "")
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
        // 包括尺寸变了（kErrResized）：交给调用方 reload 后重试，还失败就重新测速
        LogError << "ModAvdExtras capture failed" << VAR(ret) << VAR(strerror_func_ ? strerror_func_(ret) : "");
        return std::nullopt;
    }

    return image;
}

bool ModAvdExtras::load_modavd_library()
{
    // LibraryHolder 自己补平台的前后缀：extras/modavd_extras.dll、extras/libmodavd_extras.so / .dylib
    auto lib_path = modavd_root_ / "extras" / "modavd_extras";
    if (!load_library(lib_path)) {
        LogError << "Failed to load ModAVD extras library" << VAR(lib_path);
        return false;
    }
    library_loaded_ = true;
    LogInfo << "Successfully loaded ModAVD extras library from: " << lib_path;

    api_version_func_ = get_function<uint32_t()>(kApiVersionFuncName);
    open_func_ = get_function<int(const OpenParams*, void**)>(kOpenFuncName);
    close_func_ = get_function<void(void*)>(kCloseFuncName);
    capabilities_func_ = get_function<uint32_t(void*)>(kCapabilitiesFuncName);
    capture_func_ = get_function<int(void*, int, uint32_t, uint32_t, void*, size_t, FrameInfo*)>(kCaptureFuncName);
    strerror_func_ = get_function<const char*(int)>(kStrErrorFuncName);

    if (!api_version_func_ || !open_func_ || !close_func_ || !capabilities_func_ || !capture_func_) {
        LogError << "ModAVD extras library is missing functions";
        return false;
    }

    // 接口版本只增不减，1 起就有截图
    uint32_t api_version = api_version_func_();
    LogInfo << "ModAVD extras" << VAR(api_version);
    if (api_version < 1) {
        LogError << "ModAVD extras library is too old" << VAR(api_version);
        return false;
    }
    return true;
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
