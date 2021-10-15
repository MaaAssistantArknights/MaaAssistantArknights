#include "InfrastShiftTask.h"

#include "Resource.h"
#include "Controller.h"
#include "InfrastSkillsImageAnalyzer.h"

bool asst::InfrastShiftTask::run()
{
    json::value task_start_json = json::object{
        { "task_type",  "InfrastShiftTask" },
        { "task_chain", m_task_chain}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    const auto& image = ctrler.get_image();

    InfrastSkillsImageAnalyzer skills_analyzer(image);

    if (skills_analyzer.analyze()) {
    }

    return false;
}