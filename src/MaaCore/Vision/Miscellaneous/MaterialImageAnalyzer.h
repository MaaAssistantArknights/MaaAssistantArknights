#pragma once

#include <functional>

#include "Vision/VisionHelper.h"

namespace asst
{
/// Matches one item icon at multiple scales; results are ordered by confidence.
class MaterialImageAnalyzer : public VisionHelper
{
public:
    struct Match
    {
        Rect rect;
        double score = 0;
        double scale = 1;
    };

    using VisionHelper::VisionHelper;

    void set_item_id(std::string item_id) { m_item_id = std::move(item_id); }

    void set_threshold(double threshold) { m_threshold = threshold; }

    void set_scales(std::vector<double> scales) { m_scales = std::move(scales); }

    void set_cancel_check(std::function<bool()> check) { m_cancel_check = std::move(check); }

    void set_task_info(const std::string& task_name);

    bool analyze();

    const std::vector<Match>& get_result() const noexcept { return m_result; }

private:
    std::string m_item_id;
    double m_threshold = 0.8;
    std::vector<double> m_scales { 1.0 };
    std::vector<Match> m_result;
    std::function<bool()> m_cancel_check;
};
}
