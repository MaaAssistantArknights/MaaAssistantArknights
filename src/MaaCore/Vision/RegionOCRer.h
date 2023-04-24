#pragma once
#include "OCRer.h"

MAA_VISION_NS_BEGIN

class RegionOCRer : public VisionHelper, public OCRerConfig
{
public:
    using Result = OCRer::Result;
    using ResultOpt = std::optional<Result>;

public:
    using VisionHelper::VisionHelper;
    virtual ~RegionOCRer() override = default;

    ResultOpt analyze() const;
    Result result() const { return m_result; }

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

private:
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    mutable Result m_result;
};

MAA_VISION_NS_END
