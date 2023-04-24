#pragma once
#include "VisionHelper.h"

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Vision/Config/OCRerConfig.h"

MAA_NS_BEGIN

class OCRer : public VisionHelper, public OCRerConfig
{
public:
    using Result = OcrPack::Result;
    using ResultsVec = OcrPack::ResultsVec;
    using ResultsVecOpt = std::optional<ResultsVec>;

public:
    using VisionHelper::VisionHelper;
    virtual ~OCRer() override = default;

    ResultsVecOpt analyze() const;

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

protected:
    void postproc_rect_(Result& res);
    void postproc_trim_(Result& res);
    void postproc_equivalence_(Result& res);
    void postproc_replace_(Result& res);

    bool filter_and_replace_by_required_(Result& res);
};

MAA_NS_END
