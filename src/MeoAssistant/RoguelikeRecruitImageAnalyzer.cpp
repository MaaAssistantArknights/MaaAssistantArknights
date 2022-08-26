#include "RoguelikeRecruitImageAnalyzer.h"

#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "OcrWithFlagTemplImageAnalyzer.h"
#include "Resource.h"
#include "TaskData.h"

bool asst::RoguelikeRecruitImageAnalyzer::analyze()
{
    LogTraceFunction;

    OcrWithFlagTemplImageAnalyzer analyzer(m_image);
    analyzer.set_task_info("Roguelike1RecruitOcrFlag", "Roguelike1RecruitOcr");
    analyzer.set_replace(Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map);

    if (!analyzer.analyze()) {
        return false;
    }

    for (const auto& [_, rect, name] : analyzer.get_result()) {
        int elite = match_elite(rect);
        int level = match_level(rect);

        BattleRecruitOperInfo info;
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
    LogTraceFunction;

    static const std::unordered_map<std::string, int> EliteTaskName = { { "Roguelike1RecruitElite0", 0 },
                                                                        { "Roguelike1RecruitElite1", 1 },
                                                                        { "Roguelike1RecruitElite2", 2 } };

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

        if (double score = analyzer.get_result().score; score > max_score) {
            max_score = score;
            elite_result = elite;
        }
    }
    return elite_result;
}

int asst::RoguelikeRecruitImageAnalyzer::match_level(const Rect& raw_roi)
{
    LogTraceFunction;

    auto task_ptr = Task.get("Roguelike1RecruitLevel");
    OcrWithPreprocessImageAnalyzer analyzer(m_image, raw_roi.move(task_ptr->roi));
    auto& replace = Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map;
    analyzer.set_replace(replace);
    analyzer.set_expansion(1);

    if (!analyzer.analyze()) {
        return 0;
    }

    const std::string& level = analyzer.get_result().front().text;
    if (level.empty() || !ranges::all_of(level, [](char c) -> bool { return std::isdigit(c); })) {
        return 0;
    }
    return std::stoi(level);
}
