#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
struct MissingMaterial
{
    std::string item_id;
    int owned = 0;
    int required = 0;
};

class MaterialSynthesisImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~MaterialSynthesisImageAnalyzer() override = default;

    bool analyze();

    // Item 任务配置图标区域及 specialParams[0] 的模板缩放百分比,Quantity 任务识别完整的 owned/required。
    // 两部分共用同一张截图；配置或识别不完整时不产生可兑换缺口。
    static std::optional<MissingMaterial> observe_missing_material(
        const cv::Mat& image,
        const std::string& task_prefix,
        const std::vector<std::string>& item_ids,
        std::optional<Rect> item_roi = std::nullopt,
        std::optional<Rect> quantity_roi = std::nullopt);

    static std::optional<int> read_number(const cv::Mat& image, const std::string& task_name, bool exact = true);

    const MatchRect& get_result() const noexcept { return m_result; }

private:
    bool analyze(
        const std::string& task_name,
        const std::vector<std::string>& item_ids,
        double template_scale,
        std::optional<Rect> roi = std::nullopt);

    MatchRect m_result;
    double m_second_best_score = 0;
};
}
