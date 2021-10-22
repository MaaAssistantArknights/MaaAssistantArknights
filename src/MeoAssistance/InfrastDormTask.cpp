#include "InfrastDormTask.h"

#include "InfrastMoodImageAnalyzer.h"
#include "Resource.h"
#include "Controller.h"

bool asst::InfrastDormTask::run()
{
    json::value task_start_json = json::object{
        { "task_type",  "InfrastDormTask" },
        { "task_chain", m_task_chain}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    const auto& image = ctrler.get_image();
    InfrastMoodImageAnalyzer analyzer(image);
    analyzer.analyze();

    return false;
}