#include "RoguelikeSkillSelectionImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include "Config/Roguelike/RoguelikeRecruitConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeSkillSelectionImageAnalyzer::analyze()
{
    MultiMatcher flag_analyzer(m_image);
    flag_analyzer.set_task_info("RoguelikeSkillSelectionFlag");

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

    team_full_analyze();

    return !m_result.empty();
}

std::string asst::RoguelikeSkillSelectionImageAnalyzer::name_analyze(const Rect& roi)
{
    RegionOCRer analyzer;
    auto name_task_ptr = Task.get<OcrTaskInfo>("RoguelikeSkillSelectionName");
    analyzer.set_bin_threshold(name_task_ptr->specific_rect.x);
    analyzer.set_task_info(name_task_ptr);
    analyzer.set_image(m_image);
    analyzer.set_roi(roi.move(name_task_ptr->roi));
    analyzer.set_replace(
        std::dynamic_pointer_cast<OcrTaskInfo>(Task.get("CharsNameOcrReplace"))->replace_map,
        std::dynamic_pointer_cast<OcrTaskInfo>(Task.get("CharsNameOcrReplace"))->replace_full);

    if (!analyzer.analyze()) {
        return {};
    }
    return analyzer.get_result().text;
}

std::vector<asst::Rect> asst::RoguelikeSkillSelectionImageAnalyzer::skill_analyze(const Rect& roi)
{
    static const std::array<std::string, 3> TasksName = {
        "RoguelikeSkillSelectionMove1",
        "RoguelikeSkillSelectionMove2",
        "RoguelikeSkillSelectionMove3",
    };
    std::vector<Rect> result;
    result.reserve(TasksName.size());

    for (const std::string& task_name : TasksName) {
        result.emplace_back(roi.move(Task.get(task_name)->rect_move));
    }
    return result;
}

void asst::RoguelikeSkillSelectionImageAnalyzer::team_full_analyze()
{
    Matcher analyzer(m_image);
    analyzer.set_task_info("RoguelikeSkillSelectionTeamFull");
    m_team_full = !analyzer.analyze();
}
