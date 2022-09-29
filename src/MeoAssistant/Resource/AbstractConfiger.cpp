#include "AbstractConfiger.h"

#include <meojson/json.hpp>

#include "Utils/AsstUtils.hpp"
#include "Utils/Logger.hpp"

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
    catch (const json::exception& e) {
        Log.error("Json parse failed", path, e.what());
        return false;
    }
    catch (const std::exception& e) {
        Log.error("Json parse failed", path, e.what());
        return false;
    }
}
