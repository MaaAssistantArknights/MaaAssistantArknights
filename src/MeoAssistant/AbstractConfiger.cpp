#include "AbstractConfiger.h"

#include <meojson/json.hpp>

#include "AsstUtils.hpp"
#include "Logger.hpp"

bool asst::AbstractConfiger::load(const std::string& filename)
{
    LogTraceFunction;

#ifdef WIN32
    std::string cvt_filename = utils::utf8_to_gbk(filename);
#else
    std::string cvt_filename = filename;
#endif
    Log.info("Load:", cvt_filename);

    if (!std::filesystem::exists(cvt_filename)) {
        return false;
    }

    std::string content = utils::load_file_without_bom(cvt_filename);

    auto&& ret = json::parser::parse(content);
    if (!ret) {
        m_last_error = "json pasing error, content:" + content;
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
    return true;
}
