#pragma once

#include "Task/AbstractTask.h"

#include <vector>

#include "Vision/Miscellaneous/MaterialRequirementImageAnalyzer.h"

namespace asst
{
class MaterialRequirementRecognitionTask final : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~MaterialRequirementRecognitionTask() noexcept override = default;

protected:
    virtual bool _run() override;

private:
    void callback_analyze_result(const std::string& status);

    std::vector<MaterialRequirementInfo> m_result;
};
}
