#include "TemplResource.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "Utils/NoWarningCV.h"

#include "Utils/AsstImageIo.hpp"
#include "Utils/Logger.hpp"

void asst::TemplResource::set_load_required(std::unordered_set<std::string> required) noexcept
{
    m_templs_filename = std::move(required);
}

bool asst::TemplResource::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path);

#ifdef ASST_DEBUG
    bool some_file_not_exists = false;
#endif
    for (const std::string& filename : m_templs_filename) {
        std::filesystem::path filepath(path / asst::utils::path(filename));
        if (!filepath.has_extension()) {
            filepath.replace_extension(asst::utils::path(".png"));
        }
        if (std::filesystem::exists(filepath)) {
            cv::Mat templ = asst::imread(filepath);
            insert_or_assign_templ(filename, std::move(templ));
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

bool asst::TemplResource::exist_templ(const std::string& key) const noexcept
{
    return m_templs.contains(key);
}

const cv::Mat asst::TemplResource::get_templ(const std::string& key) const noexcept
{
    if (auto iter = m_templs.find(key); iter != m_templs.cend()) {
        return iter->second;
    }
    else {
        return cv::Mat();
    }
}

void asst::TemplResource::insert_or_assign_templ(const std::string& key, cv::Mat&& templ)
{
    m_templs.insert_or_assign(key, std::move(templ));
}
