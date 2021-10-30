#include "InfrastInfoTask.h"

#include "Controller.h"
#include "RuntimeStatus.h"
#include "Resource.h"
#include "InfrastFacilityImageAnalyzer.h"
#include "Logger.hpp"

bool asst::InfrastInfoTask::run()
{
    json::value task_start_json = json::object{
        { "task_type",  "InfrastInfoTask" },
        { "task_chain", m_task_chain}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    const auto& image = ctrler.get_image();

    InfrastFacilityImageAnalyzer analyzer(image);
    std::vector<std::string> all_facilities;
    for (auto&& [key, _value] : resource.infrast().get_facility_info()) {
        all_facilities.emplace_back(key);
    }
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