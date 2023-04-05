#include "BattleFormationImageAnalyzer.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Miscellaneous/ProcessTaskImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

bool asst::BattleFormationImageAnalyzer::analyze()
{
    LogTraceFunction;
    m_result.clear();

    ProcessTaskImageAnalyzer start_button_analyzer(m_image, Task.get("BattleStartAll")->next, nullptr);
    if (!start_button_analyzer.analyze()) {
        return false;
    }

    OcrWithFlagTemplImageAnalyzer oper_names_analyzer(m_image);
    oper_names_analyzer.set_task_info("BattleFormationOCRNameFlag", "BattleFormationOperNames");
    oper_names_analyzer.set_expansion(3);
    if (!oper_names_analyzer.analyze()) {
        return false;
    }

    auto proc_ocr_result = [&]() {
        const auto& oper_names = oper_names_analyzer.get_result();
        for (const auto& name_res : oper_names) {
            if (BattleData.is_name_invalid(name_res.text)) {
                continue;
            }
            const std::string& name = name_res.text;
            Rect base_point { name_res.rect.x + name_res.rect.width, name_res.rect.y, 0, 0 };
            Rect avatar_rect = base_point.move(Task.get("BattleFormationOperAvatarMove")->rect_move);
            cv::Mat avatar = m_image(make_rect<cv::Rect>(avatar_rect));
            m_result.emplace(name, avatar);
#ifdef ASST_DEBUG
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(name_res.rect), cv::Scalar(0, 255, 0), 2);
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(avatar_rect), cv::Scalar(0, 0, 255), 2);
            cv::putText(m_image_draw, utils::utf8_to_ansi(name), cv::Point(avatar_rect.x, avatar_rect.y - 20), 1, 1.2,
                        cv::Scalar(0, 0, 255), 2);
#endif
        }
    };

    proc_ocr_result();

    // 识别到了结果，但是确实没有可用的干员名。说明游戏UI可能是老版本的，布局不一样
    if (m_result.empty()) {
        oper_names_analyzer.set_task_info("BattleFormationOCRNameFlag", "BattleFormationOperNamesOldVersion");
        oper_names_analyzer.set_expansion(2);
        if (!oper_names_analyzer.analyze()) {
            return false;
        }
        proc_ocr_result();
    }

    return !m_result.empty();
}
