#pragma once
#include "VisionHelper.h"

#include <unordered_map>

namespace asst
{
// FIXME: 删掉这个类，以及对应的 task 类型
class Hasher : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~Hasher() override = default;

    bool analyze();

    void set_mask_range(int lower, int upper) noexcept;
    void set_mask_range(std::pair<int, int> mask_range) noexcept;
    void set_hash_templates(std::unordered_map<std::string, std::string> hash_templates) noexcept;
    void set_need_split(bool need_split) noexcept;
    void set_need_bound(bool need_bound) noexcept;

    const std::vector<std::string>& get_min_dist_name() const noexcept;
    const std::vector<std::string>& get_hash() const noexcept;

    static std::string s_hash(const cv::Mat& img);
    static int hamming(std::string hash1, std::string hash2);
    static std::vector<cv::Mat> split_bin(const cv::Mat& bin);
    static cv::Mat bound_bin(const cv::Mat& bin);

protected:
    std::pair<int, int> m_mask_range;
    std::unordered_map<std::string, std::string> m_hash_templates;
    bool m_need_split = false;
    bool m_need_bound = false;

    std::vector<std::string> m_hash_result;
    std::vector<std::string> m_min_dist_name;
};
}
