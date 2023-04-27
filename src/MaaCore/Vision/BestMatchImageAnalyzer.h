#pragma once

#include "AbstractImageAnalyzer.h"
#include "Vision/Config/MatcherConfig.h"

namespace asst
{
    class BestMatchImageAnalyzer : public AbstractImageAnalyzer, public MatcherConfig
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
                return "{ name: " + templ_info.name + ", matched: " + rect.to_string() + " }";
            }
            explicit operator std::string() const { return to_string(); }

            Rect rect;
            double score = 0.0;
            TemplInfo templ_info;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BestMatchImageAnalyzer() override = default;

        void append_templ(std::string name, const cv::Mat& templ = cv::Mat());

        ResultOpt analyze() const;
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        const auto& get_result() const { return m_result; }

    protected:
        virtual void _set_roi(const Rect& roi) override { set_roi(roi); }

    private:
        using MatcherConfig::set_templ;

        std::vector<TemplInfo> m_templs;
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        mutable Result m_result;
    };
}
