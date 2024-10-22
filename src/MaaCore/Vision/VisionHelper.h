#pragma once

#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/Platform.hpp"
#include "Utils/Ranges.hpp"

// #ifndef  ASST_DEBUG
// #define ASST_DEBUG
// #endif // ! ASST_DEBUG

namespace asst
{
class TaskData;
class Status;
class Assistant;
}

namespace asst
{
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

#ifdef ASST_DEBUG
    const cv::Mat& get_draw() const { return m_image_draw; }
#endif

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

template <typename RectTy>
inline static cv::Mat make_roi(const cv::Mat& img, const RectTy& roi)
{
    return img(make_rect<cv::Rect>(roi));
}

// | 1 2 3 4 |
// | 5 6 7 8 |
template <typename ResultsVec>
inline static void sort_by_horizontal_(ResultsVec& results)
{
    ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        // y 差距较小则理解为是同一排的，按x排序
        return std::abs(lhs.rect.y - rhs.rect.y) < 5 ? lhs.rect.x < rhs.rect.x : lhs.rect.y < rhs.rect.y;
    });
}

// | 1 3 5 7 |
// | 2 4 6 8 |
template <typename ResultsVec>
inline static void sort_by_vertical_(ResultsVec& results)
{
    ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool {
        // x 差距较小则理解为是同一排的，按y排序
        return std::abs(lhs.rect.x - rhs.rect.x) < 5 ? lhs.rect.y < rhs.rect.y : lhs.rect.x < rhs.rect.x;
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

// Non-Maximum Suppression
template <typename ResultsVec>
inline static ResultsVec NMS(ResultsVec results, double threshold = 0.7)
{
    ranges::sort(results, [](const auto& a, const auto& b) { return a.score > b.score; });

    ResultsVec nms_results;
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& box = results[i];
        if (box.score < 0.1f) {
            continue;
        }
        nms_results.emplace_back(box);
        for (size_t j = i + 1; j < results.size(); ++j) {
            auto& box2 = results[j];
            if (box2.score < 0.1f) {
                continue;
            }
            int iou_area = (make_rect<cv::Rect>(box.rect) & make_rect<cv::Rect>(box2.rect)).area();
            if (iou_area > threshold * box2.rect.area()) {
                box2.score = 0;
            }
        }
    }
    return nms_results;
}
} // namespace asst
