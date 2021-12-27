#include "AbstractConfiger.h"

#include <meojson/json.h>

#include "AsstUtils.hpp"

bool asst::AbstractConfiger::load(const std::string& filename)
{
    std::string content = utils::load_file_without_bom(filename);

    auto&& ret = json::parser::parse(content);
    if (!ret) {
        m_last_error = "json pasing error, content:" + content;
        return false;
    }

    json::value root = std::move(ret.value());

    try {
        return parse(root);
    }
    catch (json::exception& e) {
        m_last_error = std::string("json field error ") + e.what();
        return false;
    }
    return true;
}
