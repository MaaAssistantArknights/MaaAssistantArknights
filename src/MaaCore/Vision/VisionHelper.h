#pragma once

#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/Platform.hpp"
#include "Utils/Ranges.hpp"

// #ifndef  ASST_DEBUG
// #define ASST_DEBUG
// #endif // ! ASST_DEBUG

MAA_VISION_NS_BEGIN

class TaskData;
class Status;
class Assistant;

class VisionHelper : protected InstHelper
{
public:
    VisionHelper() = default;
    VisionHelper(const cv::Mat& image, const Rect& roi = Rect(), Assistant* inst = nullptr);
    virtual ~VisionHelper() = default;

    virtual void set_image(const cv::Mat& image);
    virtual void set_roi(const Rect& roi);
    virtual void set_log_tracing(bool enable);

    bool save_img(const std::filesystem::path& relative_dir = utils::path("debug"));

protected:
    using InstHelper::status;

protected:
    static Rect correct_rect(const Rect& rect, const cv::Mat& image);

    cv::Mat m_image;
#ifdef ASST_DEBUG
    cv::Mat m_image_draw;
#endif
    Rect m_roi;
    bool m_log_tracing = true;

private:
    using InstHelper::ctrler;
    using InstHelper::need_exit;
};

// | 1 2 3 4 |
// | 5 6 7 8 |
template <typename ResultsVec>
inline static void sort_by_horizontal_(ResultsVec& results)
{
    ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        // y 差距较小则理解为是同一排的，按x排序
        return std::abs(lhs.rect.y - rhs.rect.y) < 5 ? return lhs.rect.x < rhs.rect.x : return lhs.rect.y < rhs.rect.y;
    });
}

// | 1 3 5 7 |
// | 2 4 6 8 |
template <typename ResultsVec>
inline static void sort_by_vertical_(ResultsVec& results)
{
    ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        // x 差距较小则理解为是同一排的，按y排序
        return std::abs(lhs.rect.x - rhs.rect.x) < 5 ? return lhs.rect.y < rhs.rect.y : return lhs.rect.x < rhs.rect.x;
    });
}

template <typename ResultsVec>
inline static void sort_by_score_(ResultsVec& results)
{
    ranges::sort(results, std::greater {}, std::mem_fn(&ResultsVec::value_type::score));
}

template <typename ResultsVec>
inline static void sort_by_required_(ResultsVec& results, const std::vector<std::string>& required)
{
    std::unordered_map<std::string, size_t> req_cache;
    for (size_t i = 0; i != required.size(); ++i) {
        req_cache.emplace(required.at(i), i + 1);
    }

    // 不在 required 中的将被排在最后
    ranges::sort(results, [&req_cache](const auto& lhs, const auto& rhs) -> bool {
        size_t lvalue = req_cache[lhs.text];
        size_t rvalue = req_cache[rhs.text];
        if (lvalue == 0) {
            return false;
        }
        else if (rvalue == 0) {
            return true;
        }
        return lvalue < rvalue;
    });
}

MAA_VISION_NS_END
