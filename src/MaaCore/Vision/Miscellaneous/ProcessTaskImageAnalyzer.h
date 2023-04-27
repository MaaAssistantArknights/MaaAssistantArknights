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

        ResultOpt analyze() const;

    private:
        MatchImageAnalyzer::ResultOpt match(const std::shared_ptr<TaskInfo>& task_ptr) const;
        OcrImageAnalyzer::ResultsVecOpt ocr(const std::shared_ptr<TaskInfo>& task_ptr) const;

        std::vector<std::string> m_tasks_name;
    };
}
