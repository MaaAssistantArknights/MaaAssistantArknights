#pragma once

#include "MatchImageAnalyzer.h"

namespace asst
{
    class BestMatchImageAnalyzer : public MatchImageAnalyzer
    {
    public:
        using MatchImageAnalyzer::MatchImageAnalyzer;
        virtual ~BestMatchImageAnalyzer() override = default;

        virtual bool analyze() override;

        void append_templ(std::string name, const cv::Mat& templ = cv::Mat());
        const std::string& get_result_name() const noexcept { return m_result_name; }

    private:
        using MatchImageAnalyzer::set_region_of_appeared;
        using MatchImageAnalyzer::set_templ;
        using MatchImageAnalyzer::set_templ_name;
        using MatchImageAnalyzer::set_use_cache;

        struct TemplInfo
        {
            std::string name;
            cv::Mat templ;
        };
        std::vector<TemplInfo> m_templs;
        std::string m_result_name;
    };
}
