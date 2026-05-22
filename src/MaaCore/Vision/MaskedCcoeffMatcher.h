#pragma once

#include "MaaUtils/NoWarningCVMat.hpp"

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace asst
{
class MaskedCcoeffMatcher
{
public:
    static MaskedCcoeffMatcher& get_instance();

    void sync_cache_revision(uint64_t revision);
    static std::string make_mat_cache_key(const cv::Mat& mat);

    cv::Mat match(
        const cv::Mat& image_rgb,
        const cv::Mat& templ_rgb,
        const cv::Mat& mask_u8,
        const std::string& cache_key,
        int mask_pixels);

    static bool should_fallback_to_opencv(int mask_pixels, int result_positions);

private:
    struct TemplatePlan;

    struct CacheEntry
    {
        std::shared_ptr<const TemplatePlan> plan;
        std::list<std::string>::iterator lru_it;
        size_t bytes;
    };

    static void fnv1a_update(uint64_t& h, const void* data, size_t size);
    static size_t calc_plan_bytes(const std::string& key, const TemplatePlan& plan);

    std::shared_ptr<const TemplatePlan> get_or_build_template_plan(
        const std::string& cache_key,
        const cv::Mat& templ_f32,
        const cv::Mat& mask_f32,
        int mask_pixels);

    static constexpr size_t k_max_cache_bytes = 64ULL * 1024 * 1024; // 64 MB

    std::mutex m_cache_mtx;
    std::list<std::string> m_lru_list;
    std::unordered_map<std::string, CacheEntry> m_template_plan_cache;
    size_t m_cache_total_bytes { 0 };
    std::atomic<uint64_t> m_cache_revision { 0 };
};
}
