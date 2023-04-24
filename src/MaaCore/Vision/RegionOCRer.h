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

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }
};

MAA_VISION_NS_END
