#pragma once

#include "Utils/NoWarningCV.h"

#include <algorithm>
#include <functional>
#include <vector>

namespace asst
{
    inline void key_color(cv::InputArray src, cv::OutputArray dst, const cv::Scalar& color,
                          const cv::Scalar& tolerance = {})
    {
        return cv::inRange(src, color - tolerance, color + tolerance, dst);
    }

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

    inline auto find_skill_ready(const cv::Mat& image, const cv::Mat& temp)
    {
        auto preprocess = [](cv::Mat &img) {
            cv::Mat mask_y2;
            key_color(img, mask_y2, { 40, 94, 103 }, { 10, 10, 10 }); // BGR
            img.setTo(cv::Scalar { 2, 216, 255 }, mask_y2);           // BGR
            cv::Mat mask_y1;
            key_color(img, mask_y1, { 2, 216, 255 }, { 20, 20, 20 });
            cv::dilate(mask_y1, mask_y1, cv::getStructuringElement(cv::MORPH_RECT, { 4, 4 }));
            cv::bitwise_not(mask_y1, mask_y1);
            img.setTo(cv::Scalar { 0, 0, 0 }, mask_y1);
        };

        cv::Mat tmp = temp.clone();
        cv::Mat img = image.clone();

        cv::Mat template_mask;
        key_color(tmp, template_mask, { 0, 255, 0 }, { 0, 0, 0 });
        cv::bitwise_not(template_mask, template_mask);

        preprocess(img);
        preprocess(tmp);

        cv::Mat match;
        cv::matchTemplate(img, tmp, match, cv::TM_SQDIFF, template_mask);
        match /= template_mask.cols * template_mask.rows;
        cv::sqrt(match, match);

        auto result = match_template_helper(
            match, { tmp.cols, tmp.rows }, [](float v) { return v < 130.; }, std::greater<> {});
        return result;
    }

} // namespace asst
