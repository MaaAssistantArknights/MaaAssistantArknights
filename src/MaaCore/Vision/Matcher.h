#pragma once
#include "VisionHelper.h"

#include "Vision/Config/MatcherConfig.h"

namespace asst
{
class Matcher : public VisionHelper, public MatcherConfig
{
public:
    using Result = MatchRect;
    using ResultOpt = std::optional<Result>;

public:
    using VisionHelper::VisionHelper;
    virtual ~Matcher() override = default;

    ResultOpt analyze() const;

    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    const auto& get_result() const noexcept { return m_result; }

public:
    struct RawResult
    {
        cv::Mat matched;
        cv::Mat templ;
        std::string templ_name;
    };

    static std::vector<RawResult> preproc_and_match(const cv::Mat& image, const MatcherConfig::Params& params);

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

private:
    // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
    mutable Result m_result;
};
}
