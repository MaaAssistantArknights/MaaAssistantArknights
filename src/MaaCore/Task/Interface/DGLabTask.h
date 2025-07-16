#pragma once
#include "Task/InterfaceTask.h"
#include <Task/Miscellaneous/BattleSpyTask.h>

namespace asst
{
class DGLabTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "DGLab";

    DGLabTask(const AsstCallback& callback, Assistant* inst);
    virtual ~DGLabTask() override = default;

    bool set_params(const json::value& params) override;

private:
    std::shared_ptr<BattleSpyTask> m_battle_spy_ptr = nullptr;
};
}
