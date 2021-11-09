#include "InfrastInfoTask.h"

#include "Controller.h"
#include "InfrastFacilityImageAnalyzer.h"
#include "Logger.hpp"
#include "Resource.h"
#include "RuntimeStatus.h"

bool asst::InfrastInfoTask::run()
{
    json::value task_start_json = json::object{
        { "task_type", "InfrastInfoTask" },
        { "task_chain", m_task_chain }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    const auto& image = ctrler.get_image();

    InfrastFacilityImageAnalyzer analyzer(image);
    analyzer.set_to_be_analyzed({ "Mfg", "Trade", "Power" });
    if (!analyzer.analyze()) {
        return false;
    }
    for (auto&& [name, res] : analyzer.get_result()) {
        std::string key = "NumOf" + name;
        int size = static_cast<int>(res.size());
        status.set(key, size);
        log.trace("InfrastInfoTask | ", key, size);
    }

    return true;
}
