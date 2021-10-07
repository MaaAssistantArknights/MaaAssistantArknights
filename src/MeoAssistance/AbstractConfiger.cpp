#include "AbstractConfiger.h"

#include <sstream>
#include <fstream>

#include <json.h>

bool asst::AbstractConfiger::load(const std::string& filename)
{
    std::ifstream ifs(filename, std::ios::in);
    if (!ifs.is_open()) {
        m_last_error = filename + " can't be opened";
        return false;
    }
    std::stringstream iss;
    iss << ifs.rdbuf();
    ifs.close();
    std::string content(iss.str());

    auto&& ret = json::parser::parse(content);
    if (!ret) {
        m_last_error = "json pasing error " + content;
        return false;
    }

    json::value root = std::move(ret.value());

    try {
        parse(root);
    }
    catch (json::exception& e) {
        m_last_error = std::string("json field error ") + e.what();
        return false;
    }
    return true;
}