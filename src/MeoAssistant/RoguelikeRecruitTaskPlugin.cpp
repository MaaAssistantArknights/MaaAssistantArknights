#include "RoguelikeRecruitTaskPlugin.h"

#include "OcrImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"

void asst::RoguelikeRecruitTaskPlugin::set_opers(std::vector<std::string> opers)
{
    m_opers = std::move(opers);
}

bool asst::RoguelikeRecruitTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1ChooseOper") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeRecruitTaskPlugin::_run()
{
    OcrImageAnalyzer analyzer(m_ctrler->get_image());
    analyzer.set_task_info("Roguelike1RecruitData");
    analyzer.set_required(m_opers);

    if (!analyzer.analyze()) {
        return false;
    }

    Rect target = analyzer.get_result().front().rect;
    m_ctrler->click(target);

    return true;
}
