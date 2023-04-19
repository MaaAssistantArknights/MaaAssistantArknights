#pragma once

#include "MatchImageAnalyzer.h"

namespace asst
{
    class BestMatchImageAnalyzer : public MatchImageAnalyzer
    {
    public:
        struct TemplInfo
        {
            std::string name;
            cv::Mat templ;
        };

        using Result = TemplInfo;
        using ResultOpt = std::optional<Result>;

    public:
        using MatchImageAnalyzer::MatchImageAnalyzer;
        virtual ~BestMatchImageAnalyzer() override = default;

        const ResultOpt& analyze();

        void append_templ(std::string name, const cv::Mat& templ = cv::Mat());
        const std::optional<Result>& result() const noexcept;

        virtual void set_log_tracing(bool enable) override;

    private:
        using MatchImageAnalyzer::set_templ;

        std::vector<TemplInfo> m_templs;
        ResultOpt m_best_result;
        bool m_best_log_tracing = true;
    };
}
