#include "AbstractConfiger.h"

#include <meojson/json.hpp>

#include "AsstUtils.hpp"
#include "Logger.hpp"

bool asst::AbstractConfiger::load(const std::string& filename)
{
    LogTraceFunction;

#ifdef _WIN32
    Log.info("Load:", utils::ansi_to_utf8(filename));
#else
    Log.info("Load:", filename);
#endif

    if (!std::filesystem::exists(filename)) {
        return false;
    }

    //std::string content = utils::load_file_without_bom(filename);

    auto&& ret = json::open(filename, true);
    if (!ret) {
        m_last_error = "json passing error, filename: " + filename;
        return false;
    }

    const auto& root = ret.value();

    try {
        return parse(root);
    }
    catch (json::exception& e) {
        m_last_error = std::string("json field error ") + e.what();
        return false;
    }
    catch (std::exception& e) {
        m_last_error = std::string("json field error ") + e.what();
        return false;
    }
}
