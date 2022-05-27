#include "RoguelikeRecruitTaskPlugin.h"

#include "OcrWithFlagTemplImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "Resource.h"
#include "Logger.hpp"

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
    LogTraceFunction;

    OcrWithFlagTemplImageAnalyzer analyzer(m_ctrler->get_image());
    analyzer.set_task_info("Roguelike1RecruitOcrFlag", "Roguelike1RecruitOcr");
    analyzer.set_replace(
    std::dynamic_pointer_cast<OcrTaskInfo>(
        Task.get("CharsNameOcrReplace"))
    ->replace_map);

    analyzer.set_required(Resrc.roguelike_recruit().get_oper_order());

    if (!analyzer.analyze()) {
        return false;
    }

    analyzer.sort_result_by_required();

    auto& res = analyzer.get_result().front();
    Log.info("Choose：", res.text);
    m_ctrler->click(res.rect);

    return true;
}
