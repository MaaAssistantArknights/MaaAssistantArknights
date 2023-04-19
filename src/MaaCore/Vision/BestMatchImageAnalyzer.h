#pragma once

#include "MatchImageAnalyzer.h"
#include "Vision/Config/MatchImageAnalyzerConfig.h"

namespace asst
{
    class BestMatchImageAnalyzer : public AbstractImageAnalyzer, public MatchImageAnalyzerConfig
    {
    public:
        struct TemplInfo
        {
            std::string name;
            cv::Mat templ;
        };

        struct Result
        {
            MatchImageAnalyzer::Result matched;
            TemplInfo templ_info;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~BestMatchImageAnalyzer() override = default;

        void append_templ(std::string name, const cv::Mat& templ = cv::Mat());

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept;

    protected:
        virtual void _set_roi(const Rect& roi) override;

    private:
        using MatchImageAnalyzerConfig::set_templ;

        std::vector<TemplInfo> m_templs;
        ResultOpt m_result;
    };
}
