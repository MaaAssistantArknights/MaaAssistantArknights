#include "ReclamationBattlePlugin.h"

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "ReclamationControlTask.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

using namespace asst;

asst::ReclamationBattlePlugin::ReclamationBattlePlugin(const AsstCallback& callback, Assistant* inst,
                                                       std::string_view task_chain)
    : AbstractTaskPlugin(callback, inst, task_chain), BattleHelper(inst)
{}

bool asst::ReclamationBattlePlugin::verify(AsstMsg, const json::value&) const
{
    // 直接调用run()
    return false;
}

ReclamationBattlePlugin& asst::ReclamationBattlePlugin::set_battle_mode(const ReclamationBattleMode& mode)
{
    m_battle_mode = mode;
    return *this;
}

bool asst::ReclamationBattlePlugin::_run()
{
    LogTraceFunction;

    wait_until_start(false);

    if (m_battle_mode == ReclamationBattleMode::Giveup) {
        return quit_action();
    }
    else if (m_battle_mode == ReclamationBattleMode::BuyWater) {
        sleep(1500); // 等待技能图标
        bool result = buy_water();
        quit_action();
        return result;
    }

    return false;
}

bool asst::ReclamationBattlePlugin::do_strategic_action(const cv::Mat&)
{
    return true;
}

bool asst::ReclamationBattlePlugin::quit_action()
{
    while (!need_exit()) {
        int retry = 0;
        while (!need_exit()) {
            bool stage1 = ProcessTask(*this, { "Reclamation@ClickExitLevel" }).set_retry_times(0).run();
            bool stage2 = ProcessTask(*this, { "Reclamation@ExitLevelConfirm" }).set_retry_times(3).run();
            Log.info(__FUNCTION__, "| click exit level perform ", stage1, stage2);
            if (stage2) break;
            retry++; // stage1==true&&stage2==false，应该是手动按了确定或是没按到退出
            if ((!stage1 && !stage2) || retry > 5) {
                // 已经到了结算界面，或是用户手动操作了
                Log.error(__FUNCTION__, "| fail to operate");
                return false;
            }
        }

        sleep(Task.get("Reclamation@BattleStart")->special_params.front());

        const auto img = ctrler()->get_image();
        bool check1 = check_in_battle(img, false);

        OCRer confirmAnalyzer(img);
        confirmAnalyzer.set_task_info("Reclamation@ExitLevelConfirm");
        bool check2 = confirmAnalyzer.analyze().has_value();

        // 出现Loading转一会儿就结算了，没结算还有error_next
        OCRer loadingAnalyzer(img);
        loadingAnalyzer.set_task_info("LoadingText");
        bool check3 = loadingAnalyzer.analyze().has_value();

        Log.info(__FUNCTION__, "| click exit level check ", check1, check2, check3);

        if (!check1 && !check2) break;
        if (check3) {
            sleep(Task.get("Reclamation@BattleStart")->special_params.at(1));
            break;
        }
    }
    return true;
}

bool asst::ReclamationBattlePlugin::buy_water()
{
    if (!communicate_with(Task.get<OcrTaskInfo>("Reclamation@Liaison")->text.front())) return false;
    if (!do_dialog_procedure(Task.get<OcrTaskInfo>("Reclamation@BuyWaterProcedure")->text)) return false;
    return true;
}

bool asst::ReclamationBattlePlugin::communicate_with(const std::string& npcName)
{
    // 在存在两个及以上npc时地图会移动，从上到下、从下到上各试一次，还不行算了
    if (communicate_with_aux(npcName, [](const MatchRect& l, const MatchRect& r) {
            return l.rect.y < r.rect.y || (l.rect.y == r.rect.y && l.rect.x < r.rect.x);
        }))
        return true;
    if (communicate_with_aux(npcName, [](const MatchRect& l, const MatchRect& r) {
            return l.rect.y > r.rect.y || (l.rect.y == r.rect.y && l.rect.x > r.rect.x);
        }))
        return true;

    Log.info(__FUNCTION__, " | ", "fail to communicate with npc");
    return false;
}

bool asst::ReclamationBattlePlugin::communicate_with_aux(
    const std::string& npcName, std::function<bool(const MatchRect&, const MatchRect&)> orderComp)
{
    std::ignore = npcName;
    std::ignore = orderComp;
    //    auto image = ctrler()->get_image();
    //    BattlefieldClassifier skillReadyAnalyzer(image);
    // analyzer.set_object_of_interest({ .skill_ready = true });
    //    if (!skillReadyAnalyzer.analyze()) {
    //        Log.info(__FUNCTION__, " | ", "no ready skills");
    //        return false;
    //    }
    //    std::vector<MatchRect> skill_results = skillReadyAnalyzer.get_result();
    //
    //    std::sort(skill_results.begin(), skill_results.end(), orderComp);
    //    for (const auto& [score, rect] : skill_results) {
    //        Rect center(rect.x + rect.width / 2,
    //                    rect.y + rect.height / 2 +
    //                    Task.get("Reclamation@SkillReadyRoleOffset")->special_params.front(), 5, 5);
    //
    //        const auto use_oper_task_ptr = Task.get("BattleUseOper");
    //        ctrler()->click(center);
    //        sleep(use_oper_task_ptr->pre_delay);
    //
    //        image = ctrler()->get_image();
    //        OCRer npcNameAnalyzer(image);
    //        npcNameAnalyzer.set_task_info("Reclamation@Liaison");
    //        npcNameAnalyzer.set_required({});
    //        if (!npcNameAnalyzer.analyze()) {
    //            // 地图发生了移动
    //            Log.info(__FUNCTION__, " | ", "map moved ");
    //            break;
    //        }
    //        if (npcNameAnalyzer.get_result().front() != npcName) {
    //            // npc名称不正确
    //            Log.info(__FUNCTION__, " | ", "npc name not match ", npcNameAnalyzer.get_result().front());
    //            cancel_oper_selection();
    //            continue;
    //        }
    //
    //        ProcessTask skill_task(this_task(), { "Reclamation@BattleSkillReadyOnClick" });
    //        skill_task.set_task_delay(0);
    //
    //        bool ret = skill_task.set_retry_times(5).run();
    //        if (!ret) {
    //            cancel_oper_selection();
    //            Log.info(__FUNCTION__, " | ", "fail to click skill of npc");
    //            return false;
    //        }
    //
    //        return true;
    //    }

    return false;
}

bool asst::ReclamationBattlePlugin::do_dialog_procedure(const std::vector<std::string>& procedure)
{
    for (auto& step : procedure) {
        if (step == "Skip") {
            ProcessTask(this_task(), { "Reclamation@DialogSkip" }).set_task_delay(0).set_retry_times(5).run();
        }
        else {
            const int max_retry = 5;
            int retry = 0;
            bool succeed = false;
            while (!need_exit()) {
                if (retry == max_retry) break;

                const auto& image = ctrler()->get_image();
                OCRer dialogAnalyzer(image);
                dialogAnalyzer.set_task_info("Reclamation@BalckMarketDialogOption");
                dialogAnalyzer.set_required({ step });
                if (!dialogAnalyzer.analyze()) {
                    if (succeed) {
                        break;
                    }
                    else {
                        retry++;
                        continue;
                    }
                }
                const auto& rect = dialogAnalyzer.get_result().front().rect;
                ctrler()->click(rect);
                Log.info(__FUNCTION__, " | ", "perform dialog option ", step);
                sleep(Task.get("Reclamation@BalckMarketDialogOption")->post_delay);
                succeed = true;
            }
            if (retry == max_retry) {
                Log.info(__FUNCTION__, " | ", "fail to perform dialog option ", step);
                return false;
            }
        }
    }
    return true;
}
