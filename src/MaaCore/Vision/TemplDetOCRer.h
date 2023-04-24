#pragma once
#include "Config/MatcherConfig.h"
#include "Config/OCRerConfig.h"
#include "OCRer.h"

MAA_VISION_NS_BEGIN

class TemplDetOCRer : public VisionHelper, public OCRerConfig, public MatcherConfig
{
public:
    using Result = OCRer::Result;
    using ResultsVec = OCRer::ResultsVec;
    using ResultsVecOpt = OCRer::ResultsVecOpt;

public:
    using VisionHelper::VisionHelper;
    virtual ~TemplDetOCRer() override = default;

    void set_task_info(const std::string& templ_task_name, const std::string& ocr_task_name);
    void set_flag_rect_move(Rect flag_rect_move);

    ResultsVecOpt analyze() const;

protected:
    // from Config
    virtual void _set_roi(const Rect& roi) override { std::ignore = roi; }

protected:
    using MatcherConfig::set_task_info;
    using OCRerConfig::set_task_info;

private:
    Rect m_flag_rect_move;
};

MAA_VISION_NS_END
