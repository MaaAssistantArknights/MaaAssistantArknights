#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "NoWarningCV.h"
#include "NoWarningCVMat.h"

#include "File.hpp"
#include "Platform.hpp"
#include "WorkingDir.hpp"

namespace asst
{
    inline cv::Mat imread(const std::filesystem::path& path, int flags = cv::IMREAD_COLOR)
    {
        auto content = asst::utils::read_file<std::vector<uint8_t>>(path);
        return cv::imdecode(content, flags);
    }

    inline cv::Mat imread(const std::string& utf8_path, int flags = cv::IMREAD_COLOR)
    {
        return imread(asst::utils::path(utf8_path), flags);
    }

    inline bool imwrite(const std::filesystem::path& path, cv::InputArray img,
                        const std::vector<int>& params = std::vector<int>())
    {
        std::filesystem::path absolute_path;
        if (path.is_relative()) [[likely]] {
            const auto& user_dir = UserDir.get();
            absolute_path = user_dir / path;
        }
        else {
            absolute_path = path;
        }
        std::filesystem::create_directories(absolute_path.parent_path());
        std::ofstream of(absolute_path, std::ios::out | std::ios::binary);
        std::vector<uint8_t> encoded;
        auto ext = asst::utils::path_to_utf8_string(absolute_path.extension());
        if (cv::imencode(ext, img, encoded, params)) {
            of.write(reinterpret_cast<char*>(encoded.data()), encoded.size());
            return true;
        }
        return false;
    }

    inline bool imwrite(const std::string& utf8_path, cv::InputArray img,
                        const std::vector<int>& params = std::vector<int>())
    {
        return imwrite(asst::utils::path(utf8_path), img, params);
    }
} // namespace asst
