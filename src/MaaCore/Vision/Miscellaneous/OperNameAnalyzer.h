#pragma once
#include "Vision/Config/OCRerConfig.h"
#include "Vision/OCRer.h"
#include "Vision/VisionHelper.h"
#include <optional>

namespace asst
{
class OperNameAnalyzer : public VisionHelper, public OCRerConfig
{
public:
    using Result = OCRer::Result;
    using ResultOpt = std::optional<Result>;

public:
    using VisionHelper::VisionHelper;
    virtual ~OperNameAnalyzer() override = default;

    ResultOpt analyze() const;

    void set_use_raw(bool use_raw) { m_use_raw = use_raw; }

    void set_bottom_line_height(int height) { m_bottom_line_height = height; }

    void set_width_threshold(int threshold) { m_width_threshold = threshold; }

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    const auto& get_result() const noexcept { return m_result; }

protected:
    using OCRerConfig::set_without_det;

    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

private:
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    mutable Result m_result;
    bool m_use_raw = true;
    int m_bottom_line_height = 3; // 底部线条高度
    int m_width_threshold = 5;    // 宽度阈值
};
}; // namespace asst
