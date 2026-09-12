#pragma once
#include "Task/InterfaceTask.h"

#include <optional>

#include "Common/AsstBattleDef.h"
#include "Task/AutoRaise/AutoRaisePlan.h"

namespace asst
{
class AutoRaiseProcessTask;

class AutoRaiseTask final : public InterfaceTask
{
private:
    // 对应 params.plans[] 的一项。elite / skills / skill + skill_master 三选一，
    // 用哪个字段出现表示本次执行哪种养成动作；字段名即 JSON key，字段含义变化时必须同步改名。
    struct AutoRaiseTargetDto
    {
        battle::Role role = battle::Role::Unknown;
        std::string name;
        std::optional<int> elite;
        std::optional<int> skills;
        std::optional<int> skill;
        std::optional<int> skill_master;

        MEO_TOJSON(name, MEO_OPT elite, MEO_OPT skills, MEO_OPT skill, MEO_OPT skill_master);
        MEO_FROMJSON(name, MEO_OPT elite, MEO_OPT skills, MEO_OPT skill, MEO_OPT skill_master);
    };

public:
    inline static constexpr std::string_view TaskType = "AutoRaise";

    AutoRaiseTask(const AsstCallback& callback, Assistant* inst);
    virtual ~AutoRaiseTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::optional<AutoRaisePlan> parse_plan(const json::value& params);

    std::shared_ptr<AutoRaiseProcessTask> m_process_task_ptr;
};
} // namespace asst
