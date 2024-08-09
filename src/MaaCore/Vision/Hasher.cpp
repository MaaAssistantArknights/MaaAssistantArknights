#include "Hasher.h"

#include "Utils/NoWarningCV.h"

#include "Utils/Logger.hpp"

bool asst::Hasher::analyze()
{
    m_hash_result.clear();
    m_min_dist_name.clear();

    cv::Mat roi = m_image(make_rect<cv::Rect>(m_roi));

    if (m_mask_range.first != 0 || m_mask_range.second != 0) {
        cv::Mat bin;
        if (roi.channels() == 3) {
            cv::cvtColor(roi, roi, cv::COLOR_BGR2GRAY);
        }
        cv::inRange(roi, m_mask_range.first, m_mask_range.second, bin);
        roi = bin;
    }

    std::vector<cv::Mat> to_hash_vector;
    if (m_need_split) {
        to_hash_vector = split_bin(roi);
    }
    else {
        to_hash_vector.emplace_back(roi);
    }

    for (auto&& to_hash : to_hash_vector) {
        if (m_need_bound) {
            to_hash = bound_bin(to_hash);
        }
        std::string hash_result = s_hash(to_hash);
        // Log.debug(hash_result);

        int min_dist = INT_MAX;
        std::string cur_min_dist_name;
        for (auto&& [name, templ] : m_hash_templates) {
            int hm = hamming(hash_result, templ);
            // Log.debug(name, "dist:", hm);
            if (hm < min_dist) {
                cur_min_dist_name = name;
                min_dist = hm;
            }
        }
        m_min_dist_name.emplace_back(std::move(cur_min_dist_name));
        m_hash_result.emplace_back(std::move(hash_result));
    }

    return true;
}

void asst::Hasher::set_mask_range(int lower, int upper) noexcept
{
    m_mask_range = std::make_pair(lower, upper);
}

void asst::Hasher::set_mask_range(std::pair<int, int> mask_range) noexcept
{
    m_mask_range = std::move(mask_range);
}

void asst::Hasher::set_hash_templates(std::unordered_map<std::string, std::string> hash_templates) noexcept
{
    m_hash_templates = std::move(hash_templates);
}

void asst::Hasher::set_need_split(bool need_split) noexcept
{
    m_need_split = need_split;
}

void asst::Hasher::set_need_bound(bool need_bound) noexcept
{
    m_need_bound = need_bound;
}

const std::vector<std::string>& asst::Hasher::get_min_dist_name() const noexcept
{
    return m_min_dist_name;
}

const std::vector<std::string>& asst::Hasher::get_hash() const noexcept
{
    return m_hash_result;
}

std::string asst::Hasher::s_hash(const cv::Mat& img)
{
    static constexpr int HashKernelSize = 16;
    cv::Mat resized;
    cv::resize(img, resized, cv::Size(HashKernelSize, HashKernelSize));
    if (img.channels() == 3) {
        cv::Mat temp;
        cv::cvtColor(resized, temp, cv::COLOR_BGR2GRAY);
        resized = temp;
    }
    std::stringstream hash_value;
    uchar* pix = resized.data;
    int tmp_dec = 0;
    for (int ro = 0; ro < 256; ro++) {
        tmp_dec = tmp_dec << 1;
        if (*pix > 127) {
            tmp_dec++;
        }
        if (ro % 4 == 3) {
            hash_value << std::hex << tmp_dec;
            tmp_dec = 0;
        }
        pix++;
    }
    return hash_value.str();
}

std::vector<cv::Mat> asst::Hasher::split_bin(const cv::Mat& bin)
{
    std::vector<cv::Mat> result;

    int range_start = 0;
    bool started = false;
    for (int i = 0; i != bin.cols; ++i) {
        bool line_without_true = true;
        for (int j = 0; j != bin.rows; ++j) {
            cv::uint8_t value = bin.at<cv::uint8_t>(j, i);

            if (value) {
                line_without_true = false;
                if (!started) {
                    started = true;
                    range_start = i;
                }
                break;
            }
        }
        if (started && line_without_true) {
            started = false;
            cv::Mat range_img = bin(cv::Range::all(), cv::Range(range_start, i));
            result.emplace_back(range_img);
        }
    }
    if (started) {
        started = false;
        cv::Mat range_img = bin(cv::Range::all(), cv::Range(range_start, bin.cols));
        result.emplace_back(range_img);
    }

    return result;
}

cv::Mat asst::Hasher::bound_bin(const cv::Mat& bin)
{
    return bin(cv::boundingRect(bin));
}

int asst::Hasher::hamming(std::string hash1, std::string hash2)
{
    static constexpr int HammingFlags = 64;

    hash1.insert(hash1.begin(), HammingFlags - hash1.size(), '0');
    hash2.insert(hash2.begin(), HammingFlags - hash2.size(), '0');
    int dist = 0;
    for (int i = 0; i < HammingFlags; i = i + 16) {
        unsigned long long x =
            strtoull(hash1.substr(i, 16).c_str(), nullptr, 16) ^ strtoull(hash2.substr(i, 16).c_str(), nullptr, 16);
        while (x) {
            ++dist;
            x = x & (x - 1);
        }
    }
    return dist;
}
