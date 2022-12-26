#include "RoguelikeRecruitSupportAnalyzer.h"

#include <algorithm>
#include <regex>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

bool asst::RoguelikeRecruitSupportAnalyzer::analyze()
{
    LogTraceFunction;

    if (m_mode == battle::roguelike::SupportAnalyzeMode::ChooseSupportBtn) {
        // 识别“选择助战”
        OcrImageAnalyzer analyzer(m_image);
        const auto& task = Task.get<OcrTaskInfo>("RoguelikeChooseSupportBtnOcr");
        analyzer.set_roi(task->roi);
        analyzer.set_required(task->text);
        if (!analyzer.analyze()) return false;
        m_choose_support_result = analyzer.get_result().front().rect;
        Log.info(__FUNCTION__, "| ChooseSupportBtn");
        return true;
    }
    else if (m_mode == battle::roguelike::SupportAnalyzeMode::AnalyzeChars) {
        // 识别干员
        OcrImageAnalyzer analyzer(m_image);
        analyzer.set_roi(Task.get("RoguelikeRecruitSupportOcr")->roi);
        analyzer.set_required(m_required);
        if (!analyzer.analyze()) return false;

        const auto& char_name_rects = analyzer.get_result();
        const auto& task_off1 = Task.get("RoguelikeRecruitSupportOff1");
        const auto& task_off_elite = Task.get("RoguelikeRecruitSupportEliteOff");
        const auto& task_off_level = Task.get("RoguelikeRecruitSupportLevelOff");
        m_char_result.clear();
        for (const auto& char_rect : char_name_rects) {
            Rect name_rect = char_rect.rect;
            name_rect.x = name_rect.x + name_rect.width / 2;

            // 识别等级区域
            Rect level_roi = name_rect.move(task_off_level->rect_move);
            int char_level = match_level(level_roi);
            if (char_level <= 0) {
                // 等级识别失败，可能希望不足，舍弃结果
                Log.info(__FUNCTION__, "| match_level failed ", char_rect.text, char_level);
                continue;
            }

            // 判断是否为好友助战
            Rect color_roi = name_rect.move(task_off1->rect_move);
            bool is_friend = judge_is_friend(color_roi, task_off1->special_params.front());

            // 匹配精英化状态
            Rect elite_roi = name_rect.move(task_off_elite->rect_move);
            int char_elite = match_elite(elite_roi, task_off_elite->special_params.front());

            battle::roguelike::RecruitSupportCharInfo char_info {
                { char_rect.text, char_rect.rect, char_elite, char_level }, is_friend, char_elite, char_level
            };

            // 助战招募最多精一
            int rarity = BattleData.get_rarity(char_rect.text);
            if (rarity == 0) {
                // 非干员名，可能是屏幕边角，舍弃结果
                Log.info(__FUNCTION__, "| can't get rarity of `", char_rect.text, "`");
                continue;
            }
            else if (rarity <= 3) {
                // 无需处理
            }
            else {
                if (char_elite == 2) {
                    char_info.oper_info.elite = 1;
                    char_info.oper_info.level = 60 + 10 * (rarity - 4);
                }
            }

            Log.info(__FUNCTION__, "| AnalyzeChars append ", char_info.oper_info.name, char_info.oper_info.rect,
                     char_elite, char_level, is_friend, char_info.max_elite, char_info.max_level);
            m_char_result.push_back(char_info);
        }
        return !m_char_result.empty();
    }
    else if (m_mode == battle::roguelike::SupportAnalyzeMode::RefreshSupportBtn) {
        // 识别“更新助战列表”
        OcrImageAnalyzer analyzer(m_image);
        const auto& task = Task.get<OcrTaskInfo>("RoguelikeRefreshSupportBtnOcr");
        analyzer.set_roi(task->roi);
        if (!analyzer.analyze()) return false;
        const auto& results = analyzer.get_result();
        for (const auto& result : results) {
            Log.info(__FUNCTION__, "| RefreshSupportBtn parse `", result.text, "`", result.score);
            if (std::count(task->text.begin(), task->text.end(), result.text)) {
                m_refresh_result = { result.rect, false, 0 };
                return true;
            }
            else if (std::regex_match(result.text, std::regex("[0-9]{2}:[0-9]{2}:[0-9]{2}"))) {
                const auto& hour = std::atoi(result.text.substr(2).c_str());
                const auto& min = std::atoi(result.text.substr(3, 2).c_str());
                const auto& sec = std::atoi(result.text.substr(7, 2).c_str());
                m_refresh_result = { result.rect, true, 3600 * hour + 60 * min + sec };
                return true;
            }
        }
        return false;
    }

    return false;
}

int asst::RoguelikeRecruitSupportAnalyzer::match_elite(const Rect& roi, const int threshold)
{
    LogTraceFunction;

    static const std::unordered_map<std::string, int> EliteTaskName = {
        { "RoguelikeRecruitSupportElite0", 0 },
        { "RoguelikeRecruitSupportElite1", 1 },
        { "RoguelikeRecruitSupportElite2", 2 },
    };

    int elite_result = 0;
    double max_score = 0;

    // 二值化，避免干员立绘影响识别，精英化标识主体颜色是纯白色
    cv::Mat bin_img;
    cv::threshold(m_image, bin_img, threshold, 255, cv::THRESH_BINARY);

    for (const auto& [task_name, elite] : EliteTaskName) {
        MatchImageAnalyzer analyzer(bin_img);
        auto task_ptr = Task.get(task_name);
        analyzer.set_task_info(task_ptr);
        analyzer.set_roi(roi);
        analyzer.set_threshold(Task.get<MatchTaskInfo>(task_name)->templ_threshold);

        if (!analyzer.analyze()) {
            continue;
        }

        double score = analyzer.get_result().score;
        if (score > max_score) {
            max_score = score;
            elite_result = elite;
        }
    }

    Log.info(__FUNCTION__, "| ", roi, elite_result, max_score);
    return elite_result;
}

int asst::RoguelikeRecruitSupportAnalyzer::judge_is_friend(const Rect& roi, const double threshold)
{
    LogTraceFunction;

    const auto& color_img = m_image(make_rect<cv::Rect>(roi)); // 截取颜色部分

    // 用red通道判断是橙色还是蓝色，橙色为好友助战
    std::vector<cv::Mat> channels;
    split(color_img, channels);
    const auto& red_channel = channels.at(2);
    auto r_mean = cv::mean(red_channel)[0];
    bool is_friend = (r_mean > threshold);

    Log.info(__FUNCTION__, "| ", roi, r_mean, threshold);

    return is_friend;
}

int asst::RoguelikeRecruitSupportAnalyzer::match_level(const Rect& roi)
{
    LogTraceFunction;

    OcrWithPreprocessImageAnalyzer analyzer(m_image);
    analyzer.set_task_info("NumberOcrReplace");
    analyzer.set_roi(roi);
    analyzer.set_expansion(1);

    if (!analyzer.analyze()) {
        return -1;
    }

    Log.info(__FUNCTION__, "| ", roi, "`", analyzer.get_result().front().text, "`");
    const std::string& level = analyzer.get_result().front().text;
    if (level.empty() || !ranges::all_of(level, [](char c) -> bool { return std::isdigit(c); })) {
        return 0;
    }
    return std::stoi(level);
}
