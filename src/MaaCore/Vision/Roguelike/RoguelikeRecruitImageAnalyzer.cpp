#include "RoguelikeRecruitImageAnalyzer.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"
#include "Vision/TemplDetOCRer.h"

bool asst::RoguelikeRecruitImageAnalyzer::analyze()
{
    LogTraceFunction;

    TemplDetOCRer analyzer(m_image);
    analyzer.set_task_info("RoguelikeRecruitOcrFlag", "RoguelikeRecruitOcr");
    analyzer.set_replace(
        Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map,
        Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_full);
    analyzer.set_bin_threshold(Task.get("RoguelikeRecruitOcr")->specific_rect.x);

    auto result_opt = analyzer.analyze();
    if (!result_opt) {
        return false;
    }

    // using blue channel only when matching level to filter out the yellow ring
    cv::Mat bbb_image;
    {
        cv::Mat blue;
        cv::extractChannel(m_image, blue, 0);
        cv::merge(std::array { blue, blue, blue }, bbb_image);
    }

    sort_by_vertical_(*result_opt);
    for (const auto& res : *result_opt) {
        int elite = match_elite(res.rect);
        int level = match_level(bbb_image, res.rect);

        if (level < 0) {
            // 要么就是识别错了，要么这个干员希望不够，是灰色的
            // 主要用于忽略后面灰色的这种情况
            continue;
        }

        battle::roguelike::Recruitment info;
        info.rect = res.rect;
        info.name = res.text;
        info.elite = elite;
        info.level = level;

        Log.info(__FUNCTION__, info.name, elite, level, info.rect);
        m_result.emplace_back(std::move(info));
    }

    return !m_result.empty();
}

int asst::RoguelikeRecruitImageAnalyzer::match_elite(const Rect& raw_roi)
{
    LogTraceFunction;

    static const std::unordered_map<std::string, int> EliteTaskName = {
        { "RoguelikeRecruitElite0", 0 },
        { "RoguelikeRecruitElite1", 1 },
        { "RoguelikeRecruitElite2", 2 },
    };

    int elite_result = 0;
    double max_score = 0;

    for (const auto& [task_name, elite] : EliteTaskName) {
        Matcher analyzer(m_image);
        auto task_ptr = Task.get(task_name);
        analyzer.set_task_info(task_ptr);
        analyzer.set_roi(raw_roi.move(task_ptr->rect_move));

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

int asst::RoguelikeRecruitImageAnalyzer::match_level(const cv::Mat& image, const Rect& raw_roi)
{
    LogTraceFunction;

    RegionOCRer analyzer(image);
    analyzer.set_task_info("NumberOcrReplace");
    analyzer.set_roi(raw_roi.move(Task.get("RoguelikeRecruitLevel")->rect_move));
    analyzer.set_bin_expansion(1);

    if (!analyzer.analyze()) {
        return -1;
    }

    const std::string& level = analyzer.get_result().text;
    if (level.empty() || !ranges::all_of(level, [](char c) -> bool { return std::isdigit(c); })) {
        return 0;
    }
    return std::stoi(level);
}
