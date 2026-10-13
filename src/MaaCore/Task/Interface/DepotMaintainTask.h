#pragma once

#include "Task/InterfaceTask.h"
#include "Task/Miscellaneous/DepotMaintainPolicy.h"

namespace asst
{
// One queue task: recognition and each decision run on the core worker,
// avoiding the asynchronous GUI TaskChainStart / AsstSetTaskParams race.
class DepotMaintainTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "DepotMaintain";
    DepotMaintainTask(const AsstCallback& callback, Assistant* inst);
    bool set_params(const json::value& params) override;
    bool run() override;

private:
    struct Plan
    {
        std::string item;
        int target;
        std::string stage;
        bool enabled;
        bool stage_open;
        int medicine;
        int stone;
        int medicine_expire_days;
    };

    json::value m_fight_options;
    std::vector<Plan> m_plans;
    std::string m_fallback_stage;
    bool m_fallback_open = false;
    bool m_only_first = false;
    bool m_dependency_preview = false;
    void report(const std::string& status, const json::value& data = json::object {});
};
}
