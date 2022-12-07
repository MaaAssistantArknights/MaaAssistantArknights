#pragma once
#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    class InfrastClueImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastClueImageAnalyzer() override = default;

        virtual bool analyze() override;

        const std::vector<std::pair<Rect, std::string>>& get_result() const noexcept { return m_result; }

    protected:
        bool clue_detect();
        bool clue_analyze();

        bool m_need_detailed = false; // 是否需要详细分析（线索号）；false时只检测，不识别
        std::vector<std::pair<Rect, std::string>> m_result;
    };
}
