#pragma once

#include "Vision/VisionHelper.h"

#include "Common/AsstItemDef.h"

namespace asst
{
class MaterialSynthesisImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~MaterialSynthesisImageAnalyzer() override = default;

    bool analyze();

    bool analyze(
        const std::string& task_name,
        const std::vector<std::string>& item_ids,
        double template_scale,
        std::optional<Rect> roi = std::nullopt);

    // Item 任务配置图标区域及 specialParams[0] 的模板缩放百分比,Quantity 任务识别完整的 owned/required。
    // 两部分共用同一张截图；配置或识别不完整时不产生可刷取缺口。
    static std::optional<MissingMaterial> observe_missing_material(
        const cv::Mat& image,
        const std::string& task_prefix,
        const std::vector<std::string>& item_ids,
        std::optional<Rect> item_roi = std::nullopt,
        std::optional<Rect> quantity_roi = std::nullopt);

    const MatchRect& get_result() const noexcept { return m_result; }

private:
    MatchRect m_result;
    double m_second_best_score = 0;
};
}
