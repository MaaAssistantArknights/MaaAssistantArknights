#pragma once
#include "Task/InterfaceTask.h"

#include <array>
#include <optional>
#include <variant>

#include "Common/AsstBattleDef.h"

namespace asst
{
class OperProgressProcessTask;

class OperProgressTask final : public InterfaceTask
{
public:
    // 对应 params.plans[] 的一项。elite / skills / skill + skill_master 三选一，
    // 用哪个字段出现表示本次执行哪种养成动作；字段名即 JSON key，字段含义变化时必须同步改名。
    struct ProgressPlan
    {
        battle::Role role = battle::Role::Unknown;
        std::string name;
        std::optional<int> elite;
        std::optional<std::variant<int, std::array<int, 3>>> skill_level;

        MEO_TOJSON(MEO_OPT role, name, MEO_OPT elite, MEO_OPT skill_level);
        MEO_FROMJSON(MEO_OPT role, name, MEO_OPT elite, MEO_OPT skill_level);
    };

public:
    inline static constexpr std::string_view TaskType = "OperProgress";

    OperProgressTask(const AsstCallback& callback, Assistant* inst);
    virtual ~OperProgressTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<OperProgressProcessTask> m_process_task_ptr;
};
} // namespace asst
