#pragma once
#include "Vision/AbstractImageAnalyzer.h"

#include <memory>
#include <string>
#include <vector>

#include "Common/AsstTypes.h"

#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrImageAnalyzer.h"

namespace asst
{
    class ProcessTaskImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        struct Result
        {
            std::shared_ptr<TaskInfo> task_ptr;
            std::variant<MatchImageAnalyzer::Result, OcrImageAnalyzer::Result> result;
            Rect rect;
        };
        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~ProcessTaskImageAnalyzer() override = default;

        void set_tasks(std::vector<std::string> tasks_name) { m_tasks_name = std::move(tasks_name); }

        const ResultOpt& analyze();
        const ResultOpt& result() const noexcept { return m_result; }

    private:
        MatchImageAnalyzer::ResultOpt match_analyze(const std::shared_ptr<TaskInfo>& task_ptr);
        OcrImageAnalyzer::ResultsVecOpt ocr_analyze(const std::shared_ptr<TaskInfo>& task_ptr);

        std::vector<std::string> m_tasks_name;

        ResultOpt m_result;
    };
}
