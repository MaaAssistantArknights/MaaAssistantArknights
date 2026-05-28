#pragma once

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "Config/GeneralConfig.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"

namespace asst::utils
{
inline size_t filenum_ctrl(const std::filesystem::path& absolute_or_relative_dir, size_t max_files)
{
    std::filesystem::path absolute_path;
    if (absolute_or_relative_dir.is_relative()) {
        absolute_path = UserDir.get() / absolute_or_relative_dir;
    }
    else {
        absolute_path = absolute_or_relative_dir;
    }

    std::error_code dir_ec;
    if (!std::filesystem::is_directory(absolute_path, dir_ec)) {
        if (dir_ec) {
            Log.warn(__FUNCTION__, "failed to inspect debug image directory", absolute_path, dir_ec.message());
        }
        return 0;
    }

    size_t file_nums = 0;
    std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> files;

    std::error_code iter_ec;
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::directory_iterator iter(absolute_path, options, iter_ec);
    if (iter_ec) {
        Log.warn(__FUNCTION__, "failed to open debug image directory", absolute_path, iter_ec.message());
        return 0;
    }
    for (const std::filesystem::directory_iterator end; iter != end; iter.increment(iter_ec)) {
        if (iter_ec) {
            Log.warn(__FUNCTION__, "failed to iterate debug image directory", absolute_path, iter_ec.message());
            break;
        }

        const auto& file = *iter;
        std::error_code entry_ec;
        if (!file.is_regular_file(entry_ec)) {
            if (entry_ec) {
                Log.warn(__FUNCTION__, "failed to inspect debug image entry", file.path(), entry_ec.message());
            }
            continue;
        }

        const auto write_time = std::filesystem::last_write_time(file.path(), entry_ec);
        if (entry_ec) {
            Log.warn(__FUNCTION__, "failed to query debug image timestamp", file.path(), entry_ec.message());
            continue;
        }

        ++file_nums;
        files.emplace_back(write_time, file.path());
    }

    std::sort(files.begin(), files.end(), [](auto& a, auto& b) {
        if (a.first != b.first) {
            return a.first < b.first;
        }
        return a.second < b.second;
    });

    size_t excess = 0;
    if (file_nums > max_files) {
        excess = file_nums - max_files;
    }
    else {
        return 0;
    }

    size_t deleted = 0;
    for (size_t i = 0; i < excess; ++i) {
        std::error_code ec;
        if (std::filesystem::remove(files[i].second, ec)) {
            ++deleted;
        }
        else if (ec) {
            Log.warn(__FUNCTION__, "failed to remove old debug image", files[i].second, ec.message());
        }
    }

    return deleted;
}

inline bool save_debug_image(
    const cv::Mat& image,
    const std::filesystem::path& relative_dir,
    bool auto_clean,
    std::string_view description = "",
    std::string_view suffix = "",
    const std::string& ext = "png",
    const std::vector<int>& params = {})
{
    if (image.empty()) {
        return false;
    }

    static std::map<std::filesystem::path, size_t> s_save_cnt;
    static std::mutex s_mutex;

    auto norm_dir = relative_dir.lexically_normal();
    const auto& base_dir = UserDir.get();
    auto res = std::mismatch(base_dir.begin(), base_dir.end(), norm_dir.begin());
    if (norm_dir.is_relative() && res.first != base_dir.end()) {
        norm_dir = base_dir / norm_dir;
    }

    if (auto_clean) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto& cnt = s_save_cnt[norm_dir];
        if (cnt == 0) {
            filenum_ctrl(norm_dir, Config.get_options().debug.max_debug_file_num);
            cnt = 0;
        }
        cnt = (cnt + 1) % Config.get_options().debug.clean_files_freq;
    }

    std::string stem = MAA_NS::format_now_for_filename();
    std::string filename = suffix.empty() ? (stem + "_raw." + ext) : (stem + "_" + std::string(suffix) + "." + ext);
    auto relative_path = norm_dir / filename;

    if (description.empty()) {
        Log.trace("Save image", relative_path);
    }
    else {
        LogInfo << "Save" << description << "to" << relative_path;
    }

    return MAA_NS::imwrite(relative_path, image, params);
}
} // namespace asst::utils
