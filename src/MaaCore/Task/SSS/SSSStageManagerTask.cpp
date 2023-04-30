#include "SSSStageManagerTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Task/SSS/SSSBattleProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

bool asst::SSSStageManagerTask::_run()
{
    LogTraceFunction;

    preprocess_data();

    while (!need_exit()) {
        do {
        } while (!confirm_battle_complete() && !need_exit()); //

        if (need_exit()) {
            break;
        }

        auto stage_opt = analyze_stage();
        if (!stage_opt) {
            if (check_on_start_screen()) {
                // 通关了
                auto info = basic_info_with_what("SSSGamePass");
                callback(AsstMsg::SubTaskExtraInfo, info);
                return true;
            }
            auto info = basic_info_with_what("SSSSettlement");
            info["why"] = "Recognition error or JSON does not support this.";
            callback(AsstMsg::SubTaskExtraInfo, info);

            settle();
            break;
        }

        const std::string& stage_name = stage_opt.value();
        int& times = m_stage_try_times[stage_name];
        if (times <= 0) {
            auto info = basic_info_with_what("SSSSettlement");
            info["why"] = "Can't win, run!";
            callback(AsstMsg::SubTaskExtraInfo, info);

            settle();
            break;
        }
        click_start_button();

        SSSBattleProcessTask battle_task(m_callback, m_inst, m_task_chain);
        battle_task.set_stage_name(stage_name);
        battle_task.run();

        --times;
    }
    return true;
}

void asst::SSSStageManagerTask::preprocess_data()
{
    for (const auto& [name, stage] : SSSCopilot.get_data().stages_data) {
        m_stage_try_times[name] = stage.retry_times + 1;
    }
}

std::optional<std::string> asst::SSSStageManagerTask::analyze_stage()
{
    LogTraceFunction;

    RegionOCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("SSSStageNameOCR");
    if (!analyzer.analyze()) {
        return std::nullopt;
    }

    const std::string& text = analyzer.get_result().text;

    const auto& stages_data = SSSCopilot.get_data().stages_data;
    for (const auto& [name, stage_info] : stages_data) {
        if (name == text) {
            auto info = basic_info_with_what("SSSStage");
            info["details"]["stage"] = name;
            callback(AsstMsg::SubTaskExtraInfo, info);
            return name;
        }
    }
    return std::nullopt;
}

bool asst::SSSStageManagerTask::confirm_battle_complete()
{
    return ProcessTask(*this, { "SSSConfirmBattleComplete" })
        .set_times_limit("SSSStartFighting", 0)
        .set_times_limit("SSSBegin", 0)
        .run();
}

bool asst::SSSStageManagerTask::click_start_button()
{
    return ProcessTask(*this, { "SSSStartFighting", "SSSCloseTip" }).run();
}

bool asst::SSSStageManagerTask::settle()
{
    return ProcessTask(*this, { "SSSSettlement" }) //
               .run() &&
           ProcessTask(*this, { "SSSConfirmBattleComplete" })
               .set_times_limit("SSSStartFighting", 0)
               .set_times_limit("SSSBegin", 0)
               .run();
}

bool asst::SSSStageManagerTask::check_on_start_screen()
{
    return ProcessTask(*this, { "SSSBegin" }).set_task_delay(0).set_times_limit("SSSBegin", 0).set_retry_times(0).run();
}
