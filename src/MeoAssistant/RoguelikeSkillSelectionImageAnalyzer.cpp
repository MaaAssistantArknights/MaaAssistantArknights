#include "RoguelikeSkillSelectionImageAnalyzer.h"

#include "NoWarningCV.h"

#include "AsstUtils.hpp"
#include "OcrWithPreprocessImageAnalyzer.h"
#include "MultiMatchImageAnalyzer.h"
#include "TaskData.h"
#include "Logger.hpp"
#include "Resource.h"

bool asst::RoguelikeSkillSelectionImageAnalyzer::analyze()
{
    MultiMatchImageAnalyzer flag_analyzer(m_image);
    flag_analyzer.set_task_info("Roguelike1SkillSelectionFlag");

    if (!flag_analyzer.analyze()) {
        return false;
    }

    const auto& flags = flag_analyzer.get_result();

    if (flags.size() > 13) {
        // https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/669
        // 不知道为什么会匹配出来一堆结果，得分完全相同，坐标间隔还特别有规律。这种情况直接报错然后重试
        Log.error("Too many flags");
        return false;
    }

    int unknown_index = 0;
    for (const auto& flag : flags) {
        std::string name = name_analyze(flag.rect);
        auto skills = skill_analyze(flag.rect);
        if (skills.empty()) {
            continue;
        }
        if (name.empty()) {
            name = "Unknown" + std::to_string(unknown_index++);
        }
        m_result.emplace(std::move(name), std::move(skills));
    }

    return !m_result.empty();
}

std::string asst::RoguelikeSkillSelectionImageAnalyzer::name_analyze(const Rect& roi)
{
    OcrWithPreprocessImageAnalyzer analyzer;
    auto name_task_ptr = Task.get<OcrTaskInfo>("Roguelike1SkillSelectionName");
    analyzer.set_task_info(name_task_ptr);
    analyzer.set_image(m_image);
    analyzer.set_roi(roi.move(name_task_ptr->roi));
    analyzer.set_required(Resrc.roguelike_recruit().get_oper_order());
    analyzer.set_replace(
        std::dynamic_pointer_cast<OcrTaskInfo>(
            Task.get("CharsNameOcrReplace"))
        ->replace_map);

    if (!analyzer.analyze()) {
        return {};
    }
    return analyzer.get_result().front().text;
}

std::vector<asst::Rect> asst::RoguelikeSkillSelectionImageAnalyzer::skill_analyze(const Rect& roi)
{
    static const std::vector<std::string> TasksName = {
        "Roguelike1SkillSelectionMove1",
        "Roguelike1SkillSelectionMove2",
        "Roguelike1SkillSelectionMove3"
    };
    std::vector<Rect> result;
    result.reserve(TasksName.size());

    for (const std::string& task_name : TasksName) {
        result.emplace_back(roi.move(Task.get(task_name)->rect_move));
    }
    return result;
}
