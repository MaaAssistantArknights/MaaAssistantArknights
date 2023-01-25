#include "SingleStepTask.h"

#include "../SingleStep/SingleStepBattleProcessTask.h"
#include "Config/Miscellaneous/CopilotConfig.h"

asst::SingleStepTask::SingleStepTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType)
{}

bool asst::SingleStepTask::set_params(const json::value& params)
{
    std::string type = params.at("type").as_string();
    std::string subtype = params.at("subtype").as_string();
    const json::value& details = params.at("details");

    if (type == "copilot" && subtype == "action") {
        auto task = std::make_shared<SingleStepBattleProcessTask>(m_callback, m_inst, TaskType);
        // 请参考自动战斗协议
        task->set_actions(CopilotConfig::parse_actions(details));
        m_subtasks.emplace_back(std::move(task));

        return true;
    }

    return false;
}
