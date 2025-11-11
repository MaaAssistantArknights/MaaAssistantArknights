#pragma once

#include "Task/InterfaceTask.h"

namespace asst
{
class RoguelikeCoppersRecastPlugin;

class RoguelikeCoppersRecastTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "RoguelikeCoppersRecast";

    RoguelikeCoppersRecastTask(const AsstCallback& callback, Assistant* inst);
    virtual ~RoguelikeCoppersRecastTask() override = default;

    bool set_params(const json::value& params) override;

private:
    std::shared_ptr<RoguelikeCoppersRecastPlugin> m_recast_plugin_ptr = nullptr;
};
}
