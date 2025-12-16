#include "TemplResource.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"

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

    // 构建相对路径索引
    std::unordered_map<std::string, std::filesystem::path> relative_path_index;
    for (const auto& dir : search_paths) {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec || !entry.is_regular_file(ec)) {
                continue;
            }
            // 存储相对于根路径的相对路径
            auto rel_path = entry.path().lexically_relative(path);
            auto rel_path_str = rel_path.string();
            std::replace(rel_path_str.begin(), rel_path_str.end(), '\\', '/');
            relative_path_index.emplace(rel_path_str, rel_path);
        }
    }

    // 查找
    for (const std::string& name : m_load_required) {
        bool template_found = false;
        std::filesystem::path file_path(utils::path(name));
        if (!file_path.has_extension()) {
            file_path.replace_extension(asst::utils::path(".png"));
        }
        std::string search_name = file_path.string();
        std::replace(search_name.begin(), search_name.end(), '\\', '/');

        // 遍历所有文件以检查重复匹配
        for (const auto& [rel_path, full_path] : relative_path_index) {
            if (rel_path.ends_with(search_name)) {
                size_t pos = rel_path.length() - search_name.length();
                if (pos != 0 && rel_path[pos - 1] != '/') { // path!=, 且不在路径分隔符边界上
                    continue;
                }
            }

            if (template_found) {
                Log.error("Templ file exists in multiple paths:", m_templ_paths.at(name), full_path);
#ifdef ASST_DEBUG
                file_dumplicate = true;
#else
                return false;
#endif
            }
            auto path_iter = m_templ_paths.find(name);
            if (path_iter == m_templ_paths.end() || path_iter->second != full_path) {
                m_templs.erase(name);
                m_templ_paths.insert_or_assign(name, full_path);
            }
            template_found = true;
        }

        if (!template_found && !m_templ_paths.contains(name)) {
            Log.error("Templ load failed, file not exists:", name);
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

        cv::Mat templ = MAA_NS::imread(path_iter->second);
        m_templs.emplace(name, std::move(templ));
    }
    return m_templs.at(name);
}
