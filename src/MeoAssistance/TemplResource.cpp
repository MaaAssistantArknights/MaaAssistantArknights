#include "TemplResource.h"

#include <filesystem>
#include <array>
#include <string_view>

void asst::TemplResource::set_load_required(std::unordered_set<std::string> required) noexcept
{
    m_templs_filename = std::move(required);
}

bool asst::TemplResource::load(const std::string& dir)
{
    for (const std::string& filename : m_templs_filename) {
        std::string filepath = dir + "\\" + filename;
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

const cv::Mat& asst::TemplResource::get_templ(const std::string& key) const noexcept
{
    if (auto iter = m_templs.find(key);
        iter != m_templs.cend()) {
        return iter->second;
    }
    else {
        return cv::Mat();
    }
}

const std::pair<cv::Mat, cv::Rect>& asst::TemplResource::get_hist(const std::string& key) const noexcept
{
    if (auto iter = m_hists.find(key);
        iter != m_hists.cend()) {
        return iter->second;
    }
    else {
        return std::pair<cv::Mat, cv::Rect>();
    }
}

void asst::TemplResource::emplace_templ(std::string key, cv::Mat templ)
{
    m_templs.emplace(std::move(key), std::move(templ));
}

void asst::TemplResource::emplace_hist(std::string key, cv::Mat hist, cv::Rect roi)
{
    m_hists.emplace(std::move(key), std::make_pair(std::move(hist), std::move(roi)));
}

void asst::TemplResource::clear_hists() noexcept
{
    m_hists.clear();
}