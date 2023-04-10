#include "TemplResource.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

void asst::TemplResource::set_load_required(std::unordered_set<std::string> required) noexcept
{
    m_load_required = std::move(required);
}

bool asst::TemplResource::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path);

#ifdef ASST_DEBUG
    bool some_file_not_exists = false;
#endif
    for (const std::string& name : m_load_required) {
        std::filesystem::path filepath(path / asst::utils::path(name));
        if (!filepath.has_extension()) {
            filepath.replace_extension(asst::utils::path(".png"));
        }
        if (std::filesystem::exists(filepath)) {
            m_templ_paths.insert_or_assign(name, filepath);
            if (auto iter = m_templs.find(name); iter != m_templs.end()) {
                m_templs.erase(iter);
            }
        }
        else if (m_loaded) {
            continue;
        }
        else {
            Log.error("Templ load failed, file not exists", filepath);
#ifdef ASST_DEBUG
            some_file_not_exists = true;
#else
            return false;
#endif
        }
    }
#ifdef ASST_DEBUG
    if (some_file_not_exists) {
        return false;
    }
#endif
    m_loaded = true;
    return true;
}

const cv::Mat& asst::TemplResource::get_templ(const std::string& name)
{
    if (m_templs.find(name) == m_templs.cend()) {
        Log.info(__FUNCTION__, "lazy load", name);
        cv::Mat templ = asst::imread(m_templ_paths.at(name));
        m_templs.emplace(name, std::move(templ));
    }
    return m_templs.at(name);
}
