#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <type_traits>

#ifdef _WIN32
#include "Utils/Platform/SafeWindows.h"
#else
#include <dlfcn.h>
#endif

#include "Utils/Logger.hpp"


namespace asst
{
// template for ref_count
template <typename T>
class LibraryHolder
{
public:
    virtual ~LibraryHolder();

    static bool load_library(const std::filesystem::path& libname);
    static void unload_library();

    template <typename FuncT>
    static std::function<FuncT> get_function(const std::string& func_name);

protected:
    LibraryHolder() = default;

private:
    inline static std::filesystem::path libname_;
    inline static int ref_count_ = 0;
    inline static std::mutex mutex_;

#ifdef _WIN32
    inline static HMODULE module_ = nullptr;
#else
    inline static void* module_ = nullptr;
#endif
};

template <typename T>
inline LibraryHolder<T>::~LibraryHolder()
{
    unload_library();
}

template <typename T>
inline bool LibraryHolder<T>::load_library(const std::filesystem::path& libname)
{
    LogInfo << VAR(libname);

    std::unique_lock<std::mutex> lock(mutex_);

    if (module_ != nullptr) {
        if (libname_ != libname) {
            LogError << "Already loaded with different library" << VAR(libname_) << VAR(libname);
            return false;
        }

        ++ref_count_;
        LogDebug << "Already loaded" << VAR(ref_count_);
        return true;
    }

    LogInfo << "Loading library" << VAR(libname);

#ifdef _WIN32
    module_ = LoadLibrary(libname.c_str());
#else
    module_ = dlopen(libname.c_str(), RTLD_LAZY);
#endif

    if (module_ == nullptr) {
        LogError << "Failed to load library" << VAR(libname);
        return false;
    }

    libname_ = libname;
    ++ref_count_;
    return true;
}

template <typename T>
inline void LibraryHolder<T>::unload_library()
{
    LogInfo << VAR(libname_);

    std::unique_lock<std::mutex> lock(mutex_);

    if (module_ == nullptr) {
        LogDebug << "LibraryHolder already unloaded";
        return;
    }

    --ref_count_;
    if (ref_count_ > 0) {
        LogDebug << "LibraryHolder ref count" << VAR(ref_count_);
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
    ref_count_ = 0;
}

template <typename T>
template <typename FuncT>
inline std::function<FuncT> LibraryHolder<T>::get_function(const std::string& func_name)
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
