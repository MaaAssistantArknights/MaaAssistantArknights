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

#ifdef ASST_DEBUG
    bool some_file_not_exists = false;
#endif
    for (const std::string& name : m_load_required) {
        std::filesystem::path filepath(path / asst::utils::path(name));
        if (!filepath.has_extension()) {
            filepath.replace_extension(asst::utils::path(".png"));
        }

        if (std::filesystem::exists(filepath)) {
            if (auto path_iter = m_templ_paths.find(name);
                path_iter == m_templ_paths.end() || path_iter->second != filepath) {
                m_templs.erase(name);
                m_templ_paths.insert_or_assign(name, filepath);
            }
        }
        else if (m_templ_paths.contains(name)) {
            continue;
        }
        else {
            Log.error("Templ load failed, file not exists", filepath.lexically_relative(UserDir.get()));
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
