#include "TemplResource.h"

#include <array>
#include <filesystem>
#include <string_view>

void asst::TemplResource::append_load_required(std::unordered_set<std::string> required) noexcept
{
    m_templs_filename.insert(
        std::make_move_iterator(required.begin()),
        std::make_move_iterator(required.end()));
}

bool asst::TemplResource::load(const std::string& dir)
{
    for (const std::string& filename : m_templs_filename) {
        std::string filepath = dir + "/" + filename;
        if (std::filesystem::exists(filepath)) {
            cv::Mat templ = cv::imread(filepath);
            emplace_templ(filename, std::move(templ));
        }
        else {
            m_last_error = filepath + " not exists";
            return false;
        }
    }

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
        const static cv::Mat empty;
        return empty;
    }
}

void asst::TemplResource::emplace_templ(std::string key, cv::Mat templ)
{
    m_templs.emplace(std::move(key), std::move(templ));
}
