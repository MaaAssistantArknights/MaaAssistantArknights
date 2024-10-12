#include "BattleRecruitSupportAnalyzer.h"

#include <algorithm>
#include <regex>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"
#include "Vision/TemplDetOCRer.h"

bool asst::BattleRecruitSupportAnalyzer::analyze()
{
    LogTraceFunction;

    if (m_mode == battle::roguelike::SupportAnalyzeMode::AnalyzeChars) {
        // 识别干员
        OCRer analyzer(m_image);
        analyzer.set_roi(Task.get("RoguelikeRecruitSupportOcr")->roi);
        analyzer.set_required(m_required);
        analyzer.set_replace(
            Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map,
            Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_full);
        if (!analyzer.analyze()) {
            return false;
        }

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
                { char_rect.text, char_rect.rect, char_elite, char_level },
                is_friend,
                char_elite,
                char_level
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

            Log.info(
                __FUNCTION__,
                "| AnalyzeChars append ",
                char_info.oper_info.name,
                char_info.oper_info.rect,
                char_info.oper_info.elite,
                char_info.oper_info.level,
                is_friend,
                char_info.max_elite,
                char_info.max_level);
            m_char_result.push_back(char_info);
        }
        return !m_char_result.empty();
    }
    else if (m_mode == battle::roguelike::SupportAnalyzeMode::RefreshSupportBtn) {
        // 识别“更新助战列表”
        OCRer analyzer(m_image);

        // 未处在冷却时间
        analyzer.set_task_info("RoguelikeRefreshSupportBtnOcr");
        if (analyzer.analyze()) {
            m_refresh_result = { analyzer.get_result().front().rect, false, 0 };
            return true;
        }

        // 刷新冷却中
        analyzer.set_required({});
        analyzer.set_replace({ { "：", ":" } });
        if (!analyzer.analyze()) {
            Log.info(__FUNCTION__, "| RefreshSupportBtn analyse failed");
            return false;
        }
        const auto& results = analyzer.get_result();
        for (const auto& result : results) {
            Log.info(__FUNCTION__, "| RefreshSupportBtn parse `", result.text, "`", result.score);
            std::smatch match_results;
            if (std::regex_search(result.text, match_results, std::regex("[0-9]{2}:[0-9]{2}:[0-9]{2}"))) {
                const auto& match_str = match_results[0].str();
                const auto& hour = std::atoi(match_str.substr(2).c_str());
                const auto& min = std::atoi(match_str.substr(3, 2).c_str());
                const auto& sec = std::atoi(match_str.substr(7, 2).c_str());
                m_refresh_result = { result.rect, true, 3600 * hour + 60 * min + sec };
                return true;
            }
        }
        Log.info(__FUNCTION__, "| RefreshSupportBtn failed: no matched reusults");
        return false;
    }

    return false;
}

int asst::BattleRecruitSupportAnalyzer::match_elite(const Rect& roi, const int threshold)
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
        Matcher analyzer(bin_img);
        auto task_ptr = Task.get(task_name);
        analyzer.set_task_info(task_ptr);
        analyzer.set_roi(roi);
        analyzer.set_threshold(Task.get<MatchTaskInfo>(task_name)->templ_thresholds.front());

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

int asst::BattleRecruitSupportAnalyzer::judge_is_friend(const Rect& roi, const double threshold)
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

int asst::BattleRecruitSupportAnalyzer::match_level(const Rect& roi)
{
    LogTraceFunction;

    RegionOCRer analyzer(m_image);
    analyzer.set_task_info("NumberOcrReplace");
    analyzer.set_roi(roi);
    analyzer.set_bin_expansion(1);

    if (!analyzer.analyze()) {
        return -1;
    }

    Log.info(__FUNCTION__, "| ", roi, "`", analyzer.get_result().text, "`");
    const std::string& level = analyzer.get_result().text;
    if (level.empty() || !ranges::all_of(level, [](char c) -> bool { return std::isdigit(c); })) {
        return 0;
    }
    return std::stoi(level);
}
