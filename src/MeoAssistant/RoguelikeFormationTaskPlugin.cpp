#include "RoguelikeFormationTaskPlugin.h"

#include "RoguelikeFormationImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "ProcessTask.h"

bool asst::RoguelikeFormationTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1QuickFormation") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeFormationTaskPlugin::_run()
{
    RoguelikeFormationImageAnalyzer formation_analyzer(m_ctrler->get_image());

    if (!formation_analyzer.analyze()) {
        return false;
    }

    for (const auto& oper : formation_analyzer.get_result()) {
        if (oper.selected) {
            continue;
        }
        m_ctrler->click(oper.rect);
    }

    return true;
}
