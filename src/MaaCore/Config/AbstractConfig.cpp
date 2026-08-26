#include "AbstractConfig.h"

#include <meojson/json.hpp>

#include "Utils/Demangle.hpp"
#include "Utils/Logger.hpp"

bool asst::AbstractConfig::load(const json::value& json)
{
    std::string class_name = utils::demangle(typeid(*this).name());
    LogTraceScope(class_name + " :: load(json)");

#ifdef ASST_DEBUG
    // 不捕获异常，可以通过堆栈更直观的看到资源存在的问题
    return parse(json);
#else
    try {
        return parse(json);
    }
    catch (const json::exception& e) {
        Log.error("Json parse failed", e.what());
        return false;
    }
    catch (const std::exception& e) {
        Log.error("Json parse failed", e.what());
        return false;
    }
#endif
}

bool asst::AbstractConfig::load(const std::filesystem::path& path)
{
    std::string class_name = utils::demangle(typeid(*this).name());

    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        Log.error(class_name, __FUNCTION__, "file does not exist", path);
        return false;
    }
    m_path = path;

    LogTraceScope(class_name + " :: " + __FUNCTION__);

    auto ret = json::open(path, true, true);
    if (!ret) {
        Log.error("Json open failed", path);
        Log.info(path.lexically_relative(UserDir.get()));
        return false;
    }

    const auto& root = ret.value();

#ifdef ASST_DEBUG
    // 不捕获异常，可以通过堆栈更直观的看到资源存在的问题
    return parse(root);
#else
    try {
        return parse(root);
    }
    catch (const json::exception& e) {
        Log.error("Json parse failed", path, e.what());
        return false;
    }
    catch (const std::exception& e) {
        Log.error("Json parse failed", path, e.what());
        return false;
    }
#endif
}
