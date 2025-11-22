#pragma once

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <string>
#include <string_view>
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

    if (!std::filesystem::exists(absolute_path)) {
        return 0;
    }

    size_t file_nums = 0;
    std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> files;

    for (auto& file : std::filesystem::directory_iterator(absolute_path)) {
        if (file.is_regular_file()) {
            ++file_nums;
            files.emplace_back(std::filesystem::last_write_time(file.path()), file.path());
        }
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
        if (std::filesystem::remove(files[i].second)) {
            ++deleted;
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
