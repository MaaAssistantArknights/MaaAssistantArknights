#pragma once

#include "Utils/NoWarningCV.h"

#include <algorithm>
#include <functional>
#include <vector>

namespace asst
{
    template <typename Pred, typename Compare = std::less<>>
    inline auto match_template_helper(
        const cv::Mat1f& image, const cv::Size& repel_box = { 0, 0 }, Pred&& filter = [](auto&&) { return true; },
        Compare&& cmp = Compare {})
    {
        using value_type = std::pair<cv::Point, double>;
        std::vector<value_type> result {};
        for (auto i = 0; i < image.cols; ++i)
            for (auto j = 0; j < image.rows; ++j) {
                const cv::Point pt = { i, j };
                const auto value = image.at<float>(pt);
                if (!filter(value)) continue;
                auto iter = std::find_if(result.rbegin(), result.rend(), [&](const value_type& pair) {
                    const auto diff = pair.first - pt;
                    return std::abs(diff.x) < repel_box.width && std::abs(diff.y) < repel_box.height;
                });
                if (iter != result.rend()) {
                    if (cmp(iter->second, value)) {
                        iter->first = pt;
                        iter->second = value;
                    }
                }
                else {
                    result.emplace_back(pt, value);
                }
            }
        return result;
    }
} // namespace asst
