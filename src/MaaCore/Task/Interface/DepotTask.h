#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class DepotRecognitionTask;

class DepotTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Depot";

    DepotTask(const AsstCallback& callback, Assistant* inst);
    virtual ~DepotTask() override = default;
    void set_strict_scan(bool value);

private:
    std::shared_ptr<DepotRecognitionTask> m_recognition_task_ptr;
};
}
