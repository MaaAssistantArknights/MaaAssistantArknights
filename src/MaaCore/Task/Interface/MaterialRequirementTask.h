#pragma once

#include "Task/InterfaceTask.h"

namespace asst
{
class MaterialRequirementRecognitionTask;

class MaterialRequirementTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "MaterialRequirement";

    MaterialRequirementTask(const AsstCallback& callback, Assistant* inst);
    virtual ~MaterialRequirementTask() override = default;

private:
    std::shared_ptr<MaterialRequirementRecognitionTask> m_recognition_task_ptr = nullptr;
};
}
