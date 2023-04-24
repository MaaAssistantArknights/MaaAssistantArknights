#pragma once
#include "VisionHelper.h"

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Vision/Config/OCRerConfig.h"

MAA_VISION_NS_BEGIN

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
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    ResultsVec get_result() const { return m_results; }

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

    // Not working for OCR with detection
    using OCRerConfig::set_bin_expansion;
    using OCRerConfig::set_bin_threshold;

protected:
    void postproc_rect_(Result& res) const;
    void postproc_trim_(Result& res) const;
    void postproc_equivalence_(Result& res) const;
    void postproc_replace_(Result& res) const;

    bool filter_and_replace_by_required_(Result& res) const;

private:
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    mutable ResultsVec m_results;
};

MAA_VISION_NS_END
