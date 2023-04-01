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

    public:
        using MatchImageAnalyzer::MatchImageAnalyzer;
        virtual ~BestMatchImageAnalyzer() override = default;

        virtual bool analyze() override;

        void append_templ(std::string name, const cv::Mat& templ = cv::Mat());
        const TemplInfo& get_result() const noexcept { return m_result; }

    private:
        using MatchImageAnalyzer::set_region_of_appeared;
        using MatchImageAnalyzer::set_templ;
        using MatchImageAnalyzer::set_templ_name;
        using MatchImageAnalyzer::set_use_cache;

        std::vector<TemplInfo> m_templs;
        TemplInfo m_result;
    };
}
