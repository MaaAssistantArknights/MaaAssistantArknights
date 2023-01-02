#include "SSSStageManagerTask.h"

#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Controller.h"
#include "Task/Miscellaneous/SSSBattleProcessTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

bool asst::SSSStageManagerTask::_run()
{
    LogTraceFunction;

    preprocess_data();
    size_t stage_count = SSSCopilot.get_data().stages_data.size();

    while (!need_exit()) {
        auto stage_opt = analyze_stage();
        if (!stage_opt) {
            // TODO: 在点完开始之后，加载动画时，还会有一个很大的关卡名，可能识别成功率会更高
            // 如果这里识别失败，可以考虑再识别一下加载动画的
            Log.error("unknown stage");
            return false;
        }

        if (!click_start_button()) {
            return false;
        }
        const auto& [stage_name, stage_index] = stage_opt.value();
        int times = m_remaining_times[stage_name] + 1;

        SSSBattleProcessTask battle_task(m_callback, m_inst, m_task_chain);
        battle_task.set_stage_index(stage_index);

        for (int i = 0; i < times; ++i) {
            if (battle_task.run()) {
                break;
            }
        }
        if (stage_index >= stage_count) {
            // TODO: 任务结束
            break;
        }
    }
    return true;
}

void asst::SSSStageManagerTask::preprocess_data()
{
    for (const auto& stage : SSSCopilot.get_data().stages_data) {
        m_remaining_times[stage.info.stage_name] = stage.retry_times;
    }
}

std::optional<asst::SSSStageManagerTask::StageInfo> asst::SSSStageManagerTask::analyze_stage()
{
    OcrWithPreprocessImageAnalyzer analyzer(ctrler()->get_image());
    analyzer.set_task_info("SSSStageNameOCR");
    if (!analyzer.analyze()) {
        return std::nullopt;
    }

    analyzer.sort_result_by_score();
    const std::string& text = analyzer.get_result().front().text;

    const auto& stages_data = SSSCopilot.get_data().stages_data;
    for (int i = 0; i != stages_data.size(); ++i) {
        if (stages_data.at(i).info.stage_name == text) {
            Log.info("cur sss stage", text, ", index", i);
            return StageInfo { .name = text, .index = i };
        }
    }

    return std::nullopt;
}

bool asst::SSSStageManagerTask::click_start_button()
{
    return ProcessTask(*this, { "SSSStartFighting", "SSSCloseTip" }).run();
}
