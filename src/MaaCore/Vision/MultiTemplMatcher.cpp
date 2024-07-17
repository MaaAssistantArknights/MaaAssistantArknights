#include "MultiTemplMatcher.h"

#include "Utils/Ranges.hpp"
#include <utility>

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"

using namespace asst;

MultiTemplMatcher::ResultsVecOpt MultiTemplMatcher::analyze() const
{
    auto match_results = Matcher::preproc_and_match(make_roi(m_image, m_roi), m_params);
    if (match_results.empty()) {
        return std::nullopt;
    }

    std::vector<Result> results;
    double threshold = m_params.templ_thres.front();

    for (const auto& [matched, templ, templ_name] : match_results) {

        if (matched.empty()) {
            continue;
        }

        int min_distance = (std::min)(templ.cols, templ.rows) / 2;
        for (int i = 0; i != matched.rows; ++i) {
            for (int j = 0; j != matched.cols; ++j) {
                auto value = matched.at<float>(i, j);
                if (value < threshold || std::isnan(value) || std::isinf(value)) {
                    continue;
                }

                Rect rect(j + m_roi.x, i + m_roi.y, templ.cols, templ.rows);
                bool need_push = true;
                // 如果有两个点离得太近，只取里面得分高的那个
                // 一般相邻的都是刚刚push进去的，这里倒序快一点
                for (auto& iter : ranges::reverse_view(results)) {
                    if (std::abs(j + m_roi.x - iter.rect.x) >= min_distance ||
                        std::abs(i + m_roi.y - iter.rect.y) >= min_distance) {
                        continue;
                    }

                    if (iter.score < value) {
                        iter.rect = rect;
                        iter.score = value;
                        iter.templ_name = templ_name;
                    } // else 这个点就放弃了
                    need_push = false;
                    break;
                }
                if (need_push) {
                    Result tmp;
                    tmp.rect = rect;
                    tmp.score = value;
                    tmp.templ_name = templ_name;
                    results.emplace_back(std::move(tmp));
                }
            }
        }
    }

    if (results.empty()) {
        return std::nullopt;
    }

#ifdef ASST_DEBUG
    for (const auto& rect : results) {
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(rect.rect), cv::Scalar(0, 0, 255), 2);
        cv::putText(m_image_draw, std::to_string(rect.score), cv::Point(rect.rect.x, rect.rect.y), 1, 1,
                    cv::Scalar(0, 0, 255));
    }
#endif

    if (m_log_tracing) {
        Log.trace("multi_templ_match_templ | ", "result:", results, "roi:", m_roi);
    }

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    m_result = std::move(results);
    return m_result;
}
