#include "SSSStageManagerTask.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Task/SSS/SSSBattleProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

bool asst::SSSStageManagerTask::_run()
{
    LogTraceFunction;

    preprocess_data();

    while (!need_exit()) {
        auto stage_opt = analyze_stage();
        if (!stage_opt) {
            // TODO: 在点完开始之后，加载动画时，还会有一个很大的关卡名，可能识别成功率会更高
            // 如果这里识别失败，可以考虑再识别一下加载动画的
            Log.warn("unknown stage, settle");
            settle();
            break;
        }

        const std::string& stage_name = stage_opt.value();
        int times = m_remaining_times[stage_name] + 1;

        SSSBattleProcessTask battle_task(m_callback, m_inst, m_task_chain);
        battle_task.set_stage_name(stage_name);

        for (int i = 0; i < times; ++i) {
            if (click_start_button() && battle_task.run() && !need_exit()) {
                break;
            }
        }

        do {
        } while (!comfirm_battle_complete() && !need_exit()); //
    }
    return true;
}

void asst::SSSStageManagerTask::preprocess_data()
{
    for (const auto& [name, stage] : SSSCopilot.get_data().stages_data) {
        m_remaining_times[name] = stage.retry_times;
    }
}

std::optional<std::string> asst::SSSStageManagerTask::analyze_stage()
{
    OcrWithPreprocessImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_task_info("SSSStageNameOCR");
    if (!analyzer.analyze()) {
        return std::nullopt;
    }

    analyzer.sort_result_by_score();
    const std::string& text = analyzer.get_result().front().text;

    const auto& stages_data = SSSCopilot.get_data().stages_data;
    for (const auto& [name, stage_info] : stages_data) {
        if (name == text) {
            Log.info("cur sss stage", name);
            return name;
        }
    }
    return std::nullopt;
}

bool asst::SSSStageManagerTask::comfirm_battle_complete()
{
    return ProcessTask(*this, { "SSSComfirmBattleComplete" })
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
           ProcessTask(*this, { "SSSComfirmBattleComplete" })
               .set_times_limit("SSSStartFighting", 0)
               .set_times_limit("SSSBegin", 0)
               .run();
}
