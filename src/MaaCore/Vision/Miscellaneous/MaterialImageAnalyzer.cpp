#include "MaterialImageAnalyzer.h"

#include <cmath>
#include <tuple>

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/MultiMatcher.h"

using namespace asst;

void MaterialImageAnalyzer::set_task_info(const std::string& task_name)
{
    const auto task = Task.get<MatchTaskInfo>(task_name);
    set_roi(task->roi);
    m_threshold = task->templ_thresholds.front();
    m_scales.clear();
    for (const auto scale : task->special_params) {
        m_scales.push_back(scale / 100.0);
    }
    if (m_scales.empty()) {
        m_scales.push_back(1.0);
    }
}

bool MaterialImageAnalyzer::analyze()
{
    m_result.clear();
    if (m_image.empty() || m_roi.empty() || m_item_id.empty() || (m_cancel_check && m_cancel_check())) {
        return false;
    }
    const auto& templ = TemplResource::get_instance().get_templ(m_item_id);
    if (templ.empty()) {
        return false;
    }
    std::vector<Match> matches;
    for (const double scale : m_scales) {
        if (m_cancel_check && m_cancel_check()) {
            return false;
        }
        if (!std::isfinite(scale) || scale <= 0) {
            continue;
        }
        cv::Mat scaled;
        if (std::abs(scale - 1.0) < 1e-6) {
            scaled = templ;
        }
        else {
            cv::resize(templ, scaled, cv::Size(), scale, scale, cv::INTER_AREA);
        }
        if (scaled.cols > m_roi.width || scaled.rows > m_roi.height) {
            continue;
        }
        MultiMatcher matcher(m_image, m_roi);
        matcher.set_templ(scaled);
        matcher.set_threshold(m_threshold);
        matcher.set_method(MatchMethod::Ccoeff);
        matcher.set_mask_range(1, 255, false, true);
        matcher.set_log_tracing(false);
        const auto results = matcher.analyze();
        if (m_cancel_check && m_cancel_check()) {
            return false;
        }
        if (!results) {
            continue;
        }
        for (const auto& result : *results) {
            if (std::isfinite(result.score)) {
                matches.push_back({ result.rect, result.score, scale });
            }
        }
    }
    std::ranges::sort(matches, [](const Match& lhs, const Match& rhs) {
        if (lhs.score != rhs.score) {
            return lhs.score > rhs.score;
        }
        return std::tie(lhs.rect.y, lhs.rect.x, lhs.scale) < std::tie(rhs.rect.y, rhs.rect.x, rhs.scale);
    });
    for (const auto& match : matches) {
        const auto center = Point(match.rect.x + match.rect.width / 2, match.rect.y + match.rect.height / 2);
        if (std::ranges::none_of(m_result, [&](const Match& existing) {
                const auto other =
                    Point(existing.rect.x + existing.rect.width / 2, existing.rect.y + existing.rect.height / 2);
                return std::abs(center.x - other.x) < 30 && std::abs(center.y - other.y) < 30;
            })) {
            m_result.push_back(match);
        }
    }
    return !m_result.empty();
}
