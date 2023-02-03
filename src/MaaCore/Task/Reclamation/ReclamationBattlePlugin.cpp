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
        int retry = 0;
        while (!need_exit()) {
            bool stage1 = ProcessTask(*this, { "Reclamation@ClickExitLevel" }).set_retry_times(0).run();
            bool stage2 = ProcessTask(*this, { "Reclamation@ExitLevelConfirm" }).set_retry_times(3).run();
            Log.info(__FUNCTION__, "| click exit level perform ", stage1, stage2);
            if (stage2) break;
            retry++;    // stage1==true&&stage2==false，应该是手动按了确定或是没按到退出
            if ((!stage1 && !stage2) || retry > 5) {
                // 已经到了结算界面，或是用户手动操作了
                Log.error(__FUNCTION__, "| fail to operate");
                return ProcessTask(*this, { "Reclamation@Begin" }).run();
            }
        }

        sleep(Task.get("Reclamation@BattleStart")->special_params.front());
        
        const auto img = ctrler()->get_image();
        bool check1 = check_in_battle(img, false); 

        OcrImageAnalyzer confirmAnalyzer(img);
        confirmAnalyzer.set_task_info("Reclamation@ExitLevelConfirm");
        bool check2 = confirmAnalyzer.analyze();

        // 出现Loading转一会儿就结算了，没结算还有error_next
        OcrImageAnalyzer loadingAnalyzer(img);
        loadingAnalyzer.set_task_info("Loading");
        bool check3 = loadingAnalyzer.analyze();

        Log.info(__FUNCTION__, "| click exit level check ", check1, check2, check3);

        if ((!check1 && !check2) || check3) break;
    }

    sleep(Task.get("Reclamation@BattleStart")->special_params.at(1));
    return true;
}

bool asst::ReclamationBattlePlugin::do_strategic_action(const cv::Mat&)
{
    return true;
}
