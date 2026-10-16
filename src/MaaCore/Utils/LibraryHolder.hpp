#pragma once

#include <filesystem>
#include <functional>
#include <mutex>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#else
#include <dlfcn.h>
#endif

#include "Utils/Logger.hpp"

namespace asst
{
class LibraryHolder
{
public:
    LibraryHolder(const LibraryHolder&) = delete;
    LibraryHolder& operator=(const LibraryHolder&) = delete;
    LibraryHolder(LibraryHolder&&) = delete;
    LibraryHolder& operator=(LibraryHolder&&) = delete;

    virtual ~LibraryHolder();

    bool load_library(const std::filesystem::path& libname);
    void unload_library();

    template <typename FuncT>
    std::function<FuncT> get_function(const std::string& func_name);

protected:
    LibraryHolder() = default;

private:
    std::filesystem::path libname_;
    std::mutex mutex_;

#ifdef _WIN32
    HMODULE module_ = nullptr;
#else
    void* module_ = nullptr;
#endif
};

inline LibraryHolder::~LibraryHolder()
{
    unload_library();
}

inline bool LibraryHolder::load_library(const std::filesystem::path& libname)
{
    LogInfo << VAR(libname);

    std::unique_lock<std::mutex> lock(mutex_);

    if (module_ != nullptr) {
        if (libname_ != libname) {
            LogError << "Already loaded with different library" << VAR(libname_) << VAR(libname);
            return false;
        }

        LogDebug << "Already loaded";
        return true;
    }

    // 根据操作系统平台惯例为动态库文件名添加前后缀lib*.{dll,dylib,so}
    // 逻辑抄自boost::dll::shared_library::decorate
    std::filesystem::path filename = libname;
#ifndef _WIN32
    if (std::strncmp(libname.filename().string().c_str(), "lib", 3)) {
        filename = std::filesystem::path(
            (libname.has_parent_path() ? libname.parent_path() / "lib" : "lib").native() + libname.filename().native());
    }
#endif
#ifdef _WIN32
    filename += ".dll";
#elif defined(__APPLE__)
    filename += ".dylib";
#else
    filename += ".so";
#endif

    LogInfo << "Loading library" << VAR(libname) << VAR(filename);

#ifdef _WIN32
    module_ = LoadLibrary(filename.c_str());
    if (module_ == nullptr) {
        LogError << "LoadLibrary failed" << VAR(filename) << VAR(GetLastError());
        return false;
    }
#else
    module_ = dlopen(filename.c_str(), RTLD_LAZY);
    if (module_ == nullptr) {
        LogError << "dlopen failed" << VAR(filename) << VAR(dlerror());
        return false;
    }
#endif

    libname_ = libname;
    return true;
}

inline void LibraryHolder::unload_library()
{
    LogInfo << VAR(libname_);

    std::unique_lock<std::mutex> lock(mutex_);

    if (module_ == nullptr) {
        LogDebug << "LibraryHolder already unloaded";
        return;
    }

    LogInfo << "Unloading library" << VAR(libname_);

#ifdef _WIN32
    FreeLibrary(module_);
#else
    dlclose(module_);
#endif

    module_ = nullptr;
    libname_.clear();
}

template <typename FuncT>
inline std::function<FuncT> LibraryHolder::get_function(const std::string& func_name)
{
    LogInfo << VAR(func_name);

    std::unique_lock<std::mutex> lock(mutex_);

    if (module_ == nullptr) {
        LogError << "LibraryHolder not loaded";
        return {};
    }

#ifdef _WIN32
    auto func = GetProcAddress(module_, func_name.c_str());
#else
    auto func = dlsym(module_, func_name.c_str());
#endif

    if (func == nullptr) {
        LogError << "Failed to find exported function" << VAR(func_name);
        return {};
    }

    return reinterpret_cast<FuncT*>(func);
}

} // namespace asst
