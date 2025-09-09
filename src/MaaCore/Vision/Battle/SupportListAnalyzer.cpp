#include "SupportListAnalyzer.h"

#include "Config/TaskData.h"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::SupportListAnalyzer::analyze(const battle::Role role)
{
    LogTraceFunction;

    const auto& flag_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Flag");

    MultiMatcher flag_analyzer(m_image);
    flag_analyzer.set_task_info(flag_task_ptr);
    if (!flag_analyzer.analyze()) {
        Log.error(__FUNCTION__, "| No support unit detected");
        save_img(utils::path("debug") / utils::path("SupportListAnalyzer"));
        return false;
    }

    const auto& name_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-NameOcr");
    const auto& elite_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Elite");
    const auto& level_task_ptr = Task.get<OcrTaskInfo>("SupportListAnalyzer-LevelOcr");
    const auto& potential_task_ptr = Task.get<MatchTaskInfo>("SupportListAnalyzer-Potential");

    RegionOCRer ocr_analyzer(m_image);
    Matcher match_analyzer(m_image);
    std::optional<int> opt;
#ifndef ASST_DEBUG
    bool need_save_img = false;
#endif

    std::vector<SupportUnit> results;
    for (const auto& [rect, score, templ_name] : flag_analyzer.get_result()) {
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(rect), cv::Scalar(0, 255, 0), 2);
#endif

        // ————————————————————————————————————————————————————————————————
        // 干员名称
        // ————————————————————————————————————————————————————————————————
        const Rect name_roi = rect.move(name_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(name_roi), cv::Scalar(0, 255, 0), 2);
#endif

        ocr_analyzer.set_task_info(name_task_ptr);
        ocr_analyzer.set_roi(name_roi);
        if (!ocr_analyzer.analyze()) [[unlikely]] {
            Log.error(__FUNCTION__, "| Skip incomplete support unit");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }

        // canonical_oper_name 函数根据 role 对干员名进行消歧义，目前仅用于区分不同升变形态下的阿米娅
        const std::string name = canonical_oper_name(role, ocr_analyzer.get_result().text);

        // ————————————————————————————————————————————————————————————————
        // 干员精英阶段
        // ————————————————————————————————————————————————————————————————
        const Rect elite_roi = rect.move(elite_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(elite_roi), cv::Scalar(0, 255, 0), 2);
#endif

        match_analyzer.set_task_info(elite_task_ptr);
        match_analyzer.set_roi(elite_roi);
        int elite = 0;
        if (match_analyzer.analyze()) {
            opt = get_suffix_num(match_analyzer.get_result().templ_name);
            if (!opt) [[unlikely]] {
                Log.error(__FUNCTION__, "| Skip incomplete support unit");
#ifndef ASST_DEBUG
                need_save_img = true;
#endif
                continue;
            }
            elite = opt.value();
        }

        // ————————————————————————————————————————————————————————————————
        // 干员等级
        // ————————————————————————————————————————————————————————————————
        const Rect level_roi = rect.move(level_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(level_roi), cv::Scalar(0, 255, 0), 2);
#endif

        ocr_analyzer.set_task_info(level_task_ptr);
        ocr_analyzer.set_roi(level_roi);
        if (!ocr_analyzer.analyze()) [[unlikely]] {
            Log.error(__FUNCTION__, "| Skip incomplete support unit");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        int level = 0;
        if (!utils::chars_to_number(ocr_analyzer.get_result().text, level)) {
            Log.error(__FUNCTION__, "| Failed to convert text", ocr_analyzer.get_result().text, "to number");
            Log.error(__FUNCTION__, "| Skip incomplete support unit");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }

        // ————————————————————————————————————————————————————————————————
        // 干员潜能
        // ————————————————————————————————————————————————————————————————
        const Rect potential_roi = rect.move(potential_task_ptr->rect_move);
#ifdef ASST_DEBUG
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(potential_roi), cv::Scalar(0, 255, 0), 2);
#endif

        match_analyzer.set_task_info(potential_task_ptr);
        match_analyzer.set_roi(potential_roi);
        if (!match_analyzer.analyze()) {
            Log.error(__FUNCTION__, "| Skip incomplete support unit");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        opt = get_suffix_num(match_analyzer.get_result().templ_name);
        if (!opt) [[unlikely]] {
            Log.error(__FUNCTION__, "| Skip incomplete support unit");
#ifndef ASST_DEBUG
            need_save_img = true;
#endif
            continue;
        }
        int potential = opt.value();

        // ————————————————————————————————————————————————————————————————

        SupportUnit support_unit { .rect = rect,
                                   .name = name,
                                   .elite = elite,
                                   .level = level,
                                   .potential = potential,
                                   .from_friend = (templ_name == flag_task_ptr->templ_names.at(0)) };
        LogInfo << __FUNCTION__ << "| Found support unit" << support_unit.name << "from"
                << (support_unit.from_friend ? "a friend:" : "a stranger") << "with elite" << support_unit.elite
                << "level" << support_unit.level << "and potential" << support_unit.potential;
        results.emplace_back(std::move(support_unit));
    }
    sort_by_horizontal_(results);

#ifdef ASST_DEBUG
    save_img(utils::path("debug") / utils::path("SupportListAnalyzer"));
#else
    if (need_save_img) {
        save_img(utils::path("debug") / utils::path("SupportListAnalyzer"));
    }
#endif

    LogInfo << std::string(40, '-');
    LogInfo << "Elite"
            << "|" << "Lv"
            << "|" << "Pot"
            << "|" << "Frd"
            << "|" << "Name";
    LogInfo << std::string(40, '-');
    for (const SupportUnit& support_unit : results) {
        LogInfo << std::format("{:^5}", support_unit.elite) << "|" << std::format("{:2}", support_unit.level) << "|"
                << std::format("{:^3}", support_unit.potential) << "|"
                << std::format("{:^3}", support_unit.from_friend ? "Y" : "N") << "|" << support_unit.name;
    }
    LogInfo << std::string(40, '-');

    m_result = std::move(results);
    return !m_result.empty();
}

std::optional<int> asst::SupportListAnalyzer::get_suffix_num(const std::string& s, const char delimiter)
{
    const size_t pos = s.rfind(delimiter);
    if (pos == std::string::npos) {
        Log.error(__FUNCTION__, "| Unsupported string", s);
        return std::nullopt;
    }

    std::string num_str = s.substr(pos + 1);
    if (num_str.ends_with(".png")) {
        num_str.erase(num_str.size() - 4);
    }

    int num = 0;
    if (!utils::chars_to_number(num_str, num)) {
        Log.error(__FUNCTION__, "| Failed to convert text", num_str, "to number");
        return std::nullopt;
    }

    return num;
}
