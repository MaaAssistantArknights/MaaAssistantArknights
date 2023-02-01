#include "ReclamationBattlePlugin.h"

#include "Utils/NoWarningCV.h"

#include "Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/MatchImageAnalyzer.h"
#include "Config/TaskData.h"

asst::ReclamationBattlePlugin::ReclamationBattlePlugin(const AsstCallback& callback, Assistant* inst,
                                                           std::string_view task_chain)
    : AbstractTaskPlugin(callback, inst, task_chain), BattleHelper(inst)
{}

bool asst::ReclamationBattlePlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view == "Reclamation@BattleStart") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::ReclamationBattlePlugin::_run()
{
    LogTraceFunction;

    wait_until_start(false);

    while (!need_exit()) {
        while (!need_exit()) {
            bool stage1 = ProcessTask(*this, { "Reclamation@ClickExitLevel" }).run();
            bool stage2 = ProcessTask(*this, { "Reclamation@ExitLevelConfirm" }).run();
            if (stage2) break;
            if (!stage1 && !stage2) {
                Log.info(__FUNCTION__, "| fail to operate");
                return ProcessTask(*this, { "Reclamation@Begin" }).run();
            }
        }

        sleep(Task.get("Reclamation@BattleStart")->special_params.front());
        const auto img = ctrler()->get_image();
        bool check1 = check_in_battle(img, false);
        OcrImageAnalyzer confirmAnalyzer(img);
        confirmAnalyzer.set_task_info("Reclamation@ExitLevelConfirm");
        bool check2 = confirmAnalyzer.analyze();
        if (!check1 && !check2) break;
    }

    sleep(Task.get("Reclamation@BattleStart")->special_params.at(1));
    return true;
}

bool asst::ReclamationBattlePlugin::do_strategic_action(const cv::Mat&)
{
    return true;
}
