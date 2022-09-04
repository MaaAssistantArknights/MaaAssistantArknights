#include "AbstractConfiger.h"

#include <meojson/json.hpp>

#include "AsstUtils.hpp"
#include "Logger.hpp"

bool asst::AbstractConfiger::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path);

    if (!std::filesystem::exists(path)) {
        return false;
    }

    auto&& ret = json::open(path, true);
    if (!ret) {
        Log.error("Json open failed", path);
        return false;
    }

    const auto& root = ret.value();

    try {
        return parse(root);
    }
    catch (json::exception& e) {
        Log.error("Json parse failed", path, e.what());
        return false;
    }
    catch (std::exception& e) {
        Log.error("Json parse failed", path, e.what());
        return false;
    }
}
