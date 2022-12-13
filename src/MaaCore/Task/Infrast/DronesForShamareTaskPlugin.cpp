#include "DronesForShamareTaskPlugin.h"

#include "InfrastProductionTask.h"
#include "Task/ProcessTask.h"

bool asst::DronesForShamareTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo || details.get("subtask", std::string()) != "InfrastTradeTask") {
        return false;
    }

    if (details.at("what").as_string() == "ProductOfFacility" &&
        details.at("details").at("product").as_string() == "Money" && m_cast_ptr->get_uses_of_drone() == "Money") {
        return true;
    }
    else {
        return false;
    }
}

void asst::DronesForShamareTaskPlugin::set_task_ptr(AbstractTask* ptr)
{
    AbstractTaskPlugin::set_task_ptr(ptr);

    m_cast_ptr = dynamic_cast<InfrastProductionTask*>(ptr);
}

bool asst::DronesForShamareTaskPlugin::_run()
{
    ProcessTask shamare_task(*this, { "ShamareThumbnail" });
    shamare_task.set_retry_times(0);
    bool has_shamare = shamare_task.run();
    if (!has_shamare) {
        return true;
    }

    ProcessTask drone_task(*this, { "DroneAssistTrade" });
    drone_task.set_retry_times(5);
    bool drone_ret = drone_task.run();
    if (drone_ret) {
        m_cast_ptr->set_uses_of_drone("_Used");
    }
    return drone_ret;
}
