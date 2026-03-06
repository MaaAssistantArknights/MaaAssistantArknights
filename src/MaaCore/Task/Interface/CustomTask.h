#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class ProcessTask;
class RoguelikeCoppersRecastPlugin;

class CustomTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Custom";

    CustomTask(const AsstCallback& callback, Assistant* inst);
    virtual ~CustomTask() override = default;
    virtual bool set_params(const json::value& params) override;

    bool parse_and_register_secretfront(const std::string& task_name, std::string& resolved_task);

private:
    std::shared_ptr<ProcessTask> m_custom_task_ptr = nullptr;
    std::shared_ptr<RoguelikeCoppersRecastPlugin> m_coppers_recast_plugin_ptr = nullptr;
};
}
