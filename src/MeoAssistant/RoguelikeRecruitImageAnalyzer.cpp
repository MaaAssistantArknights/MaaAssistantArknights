#include "RoguelikeRecruitImageAnalyzer.h"

#include "OcrWithFlagTemplImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "TaskData.h"
#include "Resource.h"
#include "Logger.hpp"

bool asst::RoguelikeRecruitImageAnalyzer::analyze()
{
    OcrWithFlagTemplImageAnalyzer analyzer(m_image);
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

    for (const auto& [_, rect, name] : analyzer.get_result()) {
        int elite = match_elite(rect);
        int level = match_level(rect);

        RecruitOperInfo info;
        info.rect = rect;
        info.name = name;
        info.elite = elite;
        info.level = level;

        Log.info(__FUNCTION__, name, elite, level, rect.to_string());
        m_result.emplace_back(std::move(info));
    }

    return !m_result.empty();
}

int asst::RoguelikeRecruitImageAnalyzer::match_elite(const Rect& raw_roi)
{
    static const std::unordered_map<std::string, int> EliteTaskName = {
        { "Roguelike1RecruitElite0", 0 },
        { "Roguelike1RecruitElite1", 1 },
        { "Roguelike1RecruitElite2", 2 }
    };

    int elite_result = 0;
    double max_score = 0;

    for (const auto& [task_name, elite] : EliteTaskName) {
        MatchImageAnalyzer analyzer(m_image);
        auto task_ptr = Task.get(task_name);
        analyzer.set_task_info(task_ptr);
        analyzer.set_roi(raw_roi.move(task_ptr->roi));

        if (!analyzer.analyze()) {
            continue;
        }

        if (double score = analyzer.get_result().score;
            score > max_score) {
            max_score = score;
            elite_result = elite;
        }
    }
    return elite_result;
}

int asst::RoguelikeRecruitImageAnalyzer::match_level(const Rect& raw_roi)
{
    auto task_ptr = Task.get("Roguelike1RecruitLevel");
    OcrWithPreprocessImageAnalyzer analyzer(m_image, raw_roi.move(task_ptr->roi));
    auto& replace = std::dynamic_pointer_cast<OcrTaskInfo>(Task.get("NumberOcrReplace"))->replace_map;
    analyzer.set_replace(replace);
    analyzer.set_expansion(1);

    if (!analyzer.analyze()) {
        return 0;
    }

    std::string level = analyzer.get_result().front().text;
    return std::stoi(level);
}
