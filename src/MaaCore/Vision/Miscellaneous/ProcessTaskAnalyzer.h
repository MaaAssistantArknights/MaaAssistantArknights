#pragma once
#include "Vision/VisionHelper.h"

#include <memory>
#include <string>
#include <vector>

#include "Common/AsstTypes.h"

#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

MAA_VISION_NS_BEGIN

class ProcessTaskAnalyzer : public VisionHelper
{
public:
    struct Result
    {
        std::shared_ptr<TaskInfo> task_ptr;
        std::variant<Matcher::Result, OCRer::Result> result;
        Rect rect;
    };
    using ResultOpt = std::optional<Result>;

public:
    using VisionHelper::VisionHelper;
    virtual ~ProcessTaskAnalyzer() override = default;

    void set_tasks(std::vector<std::string> tasks_name) { m_tasks_name = std::move(tasks_name); }

    ResultOpt analyze() const;

private:
    Matcher::ResultOpt match(const std::shared_ptr<TaskInfo>& task_ptr) const;
    OCRer::ResultsVecOpt ocr(const std::shared_ptr<TaskInfo>& task_ptr) const;

    std::vector<std::string> m_tasks_name;
};

MAA_VISION_NS_END
