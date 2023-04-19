#pragma once
#include "AbstractImageAnalyzer.h"

#include "Vision/Config/MatchImageAnalyzerConfig.h"

namespace asst
{
    class MatchImageAnalyzer : public AbstractImageAnalyzer, public MatchImageAnalyzerConfig
    {
    public:
        struct Result
        {
            std::string to_string() const
            {
                return "{ rect: " + rect.to_string() + ", score: " + std::to_string(score) + " }";
            }

            Rect rect;
            double score = 0.0;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~MatchImageAnalyzer() override = default;

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept;

    public:
        struct RawResult
        {
            cv::Mat matched;
            cv::Mat templ;
            std::string templ_name;
        };
        static RawResult preproc_and_match(const cv::Mat& image, const MatchImageAnalyzerConfig::Params& params);

    protected:
        virtual void _set_roi(const Rect& roi) override;

    private:
        ResultOpt m_result;
    };
} // namespace asst
