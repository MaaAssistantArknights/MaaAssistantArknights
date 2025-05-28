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
    Log.info("load", path.lexically_relative(UserDir.get()));

    std::vector<std::filesystem::path> search_paths = { path };

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_directory()) {
            search_paths.push_back(entry.path());
            Log.debug("TemplResource::load", "Loading directory:", entry.path());
        }
    }

#ifdef ASST_DEBUG
    bool some_file_not_exists = false;
    bool file_dumplicate = false;
#endif
    for (const std::string& name : m_load_required) {
        bool found = false;

        for (const auto& dir : search_paths) {
            std::filesystem::path filepath(dir / asst::utils::path(name));
            if (!filepath.has_extension()) {
                filepath.replace_extension(asst::utils::path(".png"));
            }

            if (std::filesystem::exists(filepath)) {
                if (found) {
                    Log.error("Templ file exists in multiple directories:", m_templ_paths.at(name), filepath);
#ifdef ASST_DEBUG
                    file_dumplicate = true;
#else
                    return false;
#endif
                }
                auto path_iter = m_templ_paths.find(name);
                if (path_iter == m_templ_paths.end() || path_iter->second != filepath) {
                    m_templs.erase(name);
                    m_templ_paths.insert_or_assign(name, filepath);
                }
                found = true;
            }
        }

        if (!found && !m_templ_paths.contains(name)) {
            Log.error("Templ load failed, file not exists in all directories:", name);
#ifdef ASST_DEBUG
            some_file_not_exists = true;
#else
            return false;
#endif
        }
    }
#ifdef ASST_DEBUG
    if (some_file_not_exists || file_dumplicate) {
        return false;
    }
#endif
    return true;
}

const cv::Mat& asst::TemplResource::get_templ(const std::string& name)
{
    if (m_templs.find(name) == m_templs.cend()) {
        // Log.info(__FUNCTION__, "lazy load", name);

        auto path_iter = m_templ_paths.find(name);
        if (path_iter == m_templ_paths.cend()) {
            Log.error(__FUNCTION__, "templ not found", name);

#ifdef ASST_DEBUG
            throw std::runtime_error("templ not found: " + name);
#else
            static cv::Mat empty;
            return empty;
#endif
        }

        cv::Mat templ = asst::imread(path_iter->second);
        m_templs.emplace(name, std::move(templ));
    }
    return m_templs.at(name);
}
