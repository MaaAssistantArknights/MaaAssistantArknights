#include "SingleStepTask.h"

#include "../SingleStep/SingleStepBattleProcessTask.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Utils/Logger.hpp"

asst::SingleStepTask::SingleStepTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType)
{}

bool asst::SingleStepTask::set_params(const json::value& params)
{
    std::string type = params.get("type", "");
    std::string subtype = params.get("subtype", "");
    auto details_opt = params.find("details");

    if (type == "copilot" && subtype == "action" && details_opt) {
        auto task = std::make_shared<SingleStepBattleProcessTask>(m_callback, m_inst, TaskType);
        // for debug
        task->set_stage_name("OF-1");

        // 请参考自动战斗协议
        try {
            task->set_actions(CopilotConfig::parse_actions(*details_opt));
        }
        catch (const json::exception& e) {
            Log.error(__FUNCTION__, e.what());
            return false;
        }
        catch (const std::exception& e) {
            Log.error(__FUNCTION__, e.what());
            return false;
        }

        m_subtasks.emplace_back(std::move(task));

        return true;
    }

    return false;
}
