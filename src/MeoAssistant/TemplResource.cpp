#include "TemplResource.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "Logger.hpp"

void asst::TemplResource::append_load_required(std::unordered_set<std::string> required) noexcept
{
    LogTraceFunction;

    m_templs_filename.insert(
        std::make_move_iterator(required.begin()),
        std::make_move_iterator(required.end()));
}

bool asst::TemplResource::load(const std::string& dir)
{
    LogTraceFunction;

    for (const std::string& filename : m_templs_filename) {
        std::string filepath = dir + "/" + filename;
        if (std::filesystem::exists(filepath)) {
            cv::Mat templ = cv::imread(filepath);
            emplace_templ(filename, std::move(templ));
        }
        else if (m_loaded) {
            continue;
        }
        else {
            m_last_error = filepath + " not exists";
            return false;
        }
    }
    m_loaded = true;
    return true;
}

bool asst::TemplResource::exist_templ(const std::string& key) const noexcept
{
    return m_templs.find(key) != m_templs.cend();
}

const cv::Mat asst::TemplResource::get_templ(const std::string& key) const noexcept
{
    if (auto iter = m_templs.find(key);
        iter != m_templs.cend()) {
        return iter->second;
    }
    else {
        return cv::Mat();
    }
}

void asst::TemplResource::emplace_templ(std::string key, cv::Mat templ)
{
    m_templs[std::move(key)] = std::move(templ);
}
