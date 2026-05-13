#pragma once

#include "MaaUtils/NoWarningCVMat.hpp"

#include <atomic>
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
    struct DftPlan;

    static void fnv1a_update(uint64_t& h, const void* data, size_t size);

    std::shared_ptr<const TemplatePlan> get_or_build_template_plan(
        const std::string& cache_key,
        const cv::Mat& templ_f32,
        const cv::Mat& mask_f32,
        int mask_pixels);

    std::shared_ptr<const DftPlan> get_or_build_dft_plan(
        const std::string& cache_key,
        const TemplatePlan& template_plan,
        int dft_rows,
        int dft_cols);

    std::mutex m_cache_mtx;
    std::unordered_map<std::string, std::shared_ptr<const TemplatePlan>> m_template_plan_cache;
    std::unordered_map<std::string, std::shared_ptr<const DftPlan>> m_dft_plan_cache;
    std::atomic<uint64_t> m_cache_revision { 0 };
};
}
