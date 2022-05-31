#include "RoguelikeRecruitTaskPlugin.h"

#include "RoguelikeRecruitImageAnalyzer.h"
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

    RoguelikeRecruitImageAnalyzer analyzer(m_ctrler->get_image());

    if (!analyzer.analyze()) {
        return false;
    }

    bool recruited = false;

    auto recruit_oper = [&](const RoguelikeRecruitImageAnalyzer::RecruitOperInfo& info) {
        Log.info("Choose：", info.name, info.elite, info.level);
        m_ctrler->click(info.rect);
        recruited = true;
    };

    for (const auto& info : analyzer.get_result()) {
        if (info.elite == 0 ||
            (info.elite == 1 && info.level < 50)) {
            continue;
        }
        recruit_oper(info);
        break;
    }

    if (!recruited) {
        Log.info("All are lower");
        // 随便招个精一的
        for (const auto& info : analyzer.get_result()) {
            if (info.elite == 0) {
                continue;
            }
            recruit_oper(info);
            break;
        }
    }

    if (!recruited) {
        // 随便招个
        Log.info("All are very lower");
        auto info = analyzer.get_result().front();
        recruit_oper(info);
    }

    return recruited;
}
