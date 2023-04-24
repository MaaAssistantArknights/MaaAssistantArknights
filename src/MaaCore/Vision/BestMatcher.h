#pragma once

#include "Matcher.h"
#include "Vision/Config/MatcherConfig.h"

MAA_VISION_NS_BEGIN

class BestMatcher : public VisionHelper, public MatcherConfig
{
public:
    struct TemplInfo
    {
        std::string name;
        cv::Mat templ;
    };

    struct Result
    {
        std::string to_string() const
        {
            return "{ name: " + templ_info.name + ", matched: " + matched.to_string() + " }";
        }
        explicit operator std::string() const { return to_string(); }

        Matcher::Result matched;
        TemplInfo templ_info;
    };

    using ResultOpt = std::optional<Result>;

public:
    using VisionHelper::VisionHelper;
    virtual ~BestMatcher() override = default;

    void append_templ(std::string name, const cv::Mat& templ = cv::Mat());

    ResultOpt analyze() const;

protected:
    virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

private:
    using MatcherConfig::set_templ;

    std::vector<TemplInfo> m_templs;
};

MAA_VISION_NS_END
