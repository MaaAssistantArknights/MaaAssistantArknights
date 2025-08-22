#include "RoguelikeCoppersTaskPlugin.h"
#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"
#include "Vision/VisionHelper.h"

bool asst::RoguelikeCoppersTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    const std::string& theme = m_config->get_theme();

    // 本插件仅用于界园肉鸽
    if (theme != RoguelikeTheme::JieGarden) {
        return false;
    }

    const std::shared_ptr<MatchTaskInfo> config = Task.get<MatchTaskInfo>(theme + "@Roguelike@CoppersConfig");

    m_col = config->special_params.at(0);
    m_row = config->special_params.at(1);
    m_origin_x = config->special_params.at(2);
    m_origin_y = config->special_params.at(3);
    m_last_y = config->special_params.at(4);
    m_column_offset = config->special_params.at(5);
    m_row_offset = config->special_params.at(6);
    m_ocr_roi_offset_y = config->special_params.at(7);

    return true;
}

bool asst::RoguelikeCoppersTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& theme = m_config->get_theme();

    if (theme != RoguelikeTheme::JieGarden) {
        return false;
    }

    if (details.get("details", "task", "").ends_with("Roguelike@CoppersTakeFlag")) {
        m_run_mode = CoppersTaskRunMode::SWITCH;
        Log.info(__FUNCTION__, "| plugin activated for SWITCH mode");
    }
    else if (details.get("details", "task", "").ends_with("Roguelike@GetDropSwitch")) {
        m_run_mode = CoppersTaskRunMode::CHOOSE;
        Log.info(__FUNCTION__, "| plugin activated for CHOOSE mode");
    }
    else {
        return false;
    }

    const auto mode = m_config->get_mode();
    if (mode == RoguelikeMode::Investment) {
        return m_config->get_invest_with_more_score();
    }
    if (mode == RoguelikeMode::Collectible) {
        return m_config->get_collectible_mode_shopping();
    }

    return true;
}

void asst::RoguelikeCoppersTaskPlugin::reset_in_run_variables()
{
    m_copper_list.clear();
    m_new_copper = RoguelikeCopper();
}

bool asst::RoguelikeCoppersTaskPlugin::_run()
{
    LogTraceFunction;

    const auto& name_task_ptr = Task.get<OcrTaskInfo>("JieGarden@Roguelike@CoppersAnalyzer-NameOCR");

    switch (m_run_mode) {
    case CoppersTaskRunMode::CHOOSE: {
        MultiMatcher matcher_analyzer_choose;
        matcher_analyzer_choose.set_task_info("JieGarden@Roguelike@GetDropSwitch");
        auto choose_image = ctrler()->get_image();
        matcher_analyzer_choose.set_image(choose_image);
        if (!matcher_analyzer_choose.analyze()) {
            Log.error(__FUNCTION__, "| failed to analyze JieGarden@Roguelike@GetDropSwitch");
            return false;
        }
        auto choose_match_results = matcher_analyzer_choose.get_result();
        sort_by_horizontal_(choose_match_results);

        RegionOCRer ocr_analyzer(choose_image);

        int choose_index = 0;
        for (const auto& [rect, score, templ_name] : choose_match_results) {
            Rect roi(rect.x, rect.y + m_ocr_roi_offset_y, rect.width, rect.height);
            ocr_analyzer.set_roi(roi);
            ocr_analyzer.set_replace(name_task_ptr->replace_map, name_task_ptr->replace_full);
            if (!ocr_analyzer.analyze()) {
                Log.error(__FUNCTION__, "| no copper name is recognised at position (", choose_index, ")");
                continue;
            }

            const auto& copper_name = ocr_analyzer.get_result().text;
            if (copper_name.empty()) {
                Log.error(__FUNCTION__, "| copper name is empty at position (", choose_index, ")");
                continue;
            }
            Log.info(__FUNCTION__, "| found copper:", copper_name, "(", choose_index, ")");

            const auto& theme = m_config->get_theme();
            m_pending_copper.emplace_back(
                RoguelikeCopper(copper_name, CopperType::Unknown, 1, choose_index, theme),
                Point(rect.x + rect.width / 2, rect.y + rect.height / 2));
            choose_index++;
        }

        // 寻找 m_pending_copper 中 pickup_priority 最大的
        auto max_pickup_it =
            std::max_element(m_pending_copper.begin(), m_pending_copper.end(), [](const auto& a, const auto& b) {
                return a.first.pickup_priority < b.first.pickup_priority;
            });

        if (max_pickup_it == m_pending_copper.end()) {
            Log.error(__FUNCTION__, "| no valid copper found for pickup");
            return false;
        }

        Log.info(
            __FUNCTION__,
            "| selecting copper: ",
            max_pickup_it->first.name,
            " with priority: ",
            max_pickup_it->first.pickup_priority);
        ctrler()->click(max_pickup_it->second);
    } break;

    case CoppersTaskRunMode::SWITCH: {
        swipe_to_the_left_of_copperlist(2); // 有时候进去不在最左边

        MultiMatcher matcher_analyzer;

        for (int col = 0; col <= m_col; col++) { // 已知列表固定4列？ 10-12个通宝  每列3个
            int switch_index = 0;
            if (col == 0) {
                matcher_analyzer.set_task_info(
                    "JieGarden@Roguelike@CoppersAnalyzer-LeftType"); // 第一次识别左侧拾取的通宝
            }
            else if (col == m_col) {
                matcher_analyzer.set_task_info("JieGarden@Roguelike@CoppersAnalyzer-RightType"); // 最后一列对右侧识别
            }
            else {
                matcher_analyzer.set_task_info("JieGarden@Roguelike@CoppersAnalyzer-Type");
            }

            auto switch_image = ctrler()->get_image();
            matcher_analyzer.set_image(switch_image);

            if (!matcher_analyzer.analyze()) {
                Log.error(__FUNCTION__, "| no coppers are recognised in column", col);
                return false;
            }

            RegionOCRer ocr_analyzer(switch_image);

            auto switch_match_results = matcher_analyzer.get_result();
            sort_by_vertical_(switch_match_results); // 按照垂直方向排序（从上到下）

            for (const auto& [rect, score, templ_name] : switch_match_results) {
                switch_index++;
                auto copper_type = RoguelikeCoppersConfig::templ2type(templ_name);

                // 识别完通宝类型后将roi偏移进行通宝名称OCR
                const Rect roi = rect.move(name_task_ptr->roi);
                ocr_analyzer.set_roi(roi);
                ocr_analyzer.set_replace(name_task_ptr->replace_map, name_task_ptr->replace_full);
                if (!ocr_analyzer.analyze()) {
                    Log.error(
                        __FUNCTION__,
                        "| no copper name is recognised at position (",
                        col,
                        ",",
                        switch_index,
                        ")");
                    continue;
                }

                const auto& copper_name = ocr_analyzer.get_result().text;
                if (copper_name.empty()) {
                    Log.error(__FUNCTION__, "| copper name is empty at position (", col, ",", switch_index, ")");
                    continue;
                }
                Log.info(__FUNCTION__, "| found copper:", copper_name, "(", col, ",", switch_index, ")");

                const auto& theme = m_config->get_theme();
                if (col != 0) {
                    m_copper_list.emplace_back(copper_name, copper_type, col, switch_index, theme);
                }
                else {
                    m_new_copper = RoguelikeCopper(copper_name, copper_type, col, switch_index, theme);
                }
            }

            if (col != 0 && col != m_col) {
                slowly_swipe_to_the_right_of_copperlist(1);
            }
        }

        swipe_to_the_left_of_copperlist(m_col);

        // 寻找 m_copper_list 中 discard_priority 最大的
        auto max_discard_it = std::max_element(
            m_copper_list.begin(),
            m_copper_list.end(),
            [](const RoguelikeCopper& a, const RoguelikeCopper& b) { return a.discard_priority < b.discard_priority; });

        if (max_discard_it == m_copper_list.end()) {
            Log.error(__FUNCTION__, "| no coppers found in the list for comparison");
            return false;
        }

        if (max_discard_it->discard_priority < m_new_copper.discard_priority) { // 盒里最差的通宝都比新捡的好
            Log.info(
                __FUNCTION__,
                "| new copper (",
                m_new_copper.name,
                ") is worse than all existing coppers, skipping exchange");
            // 不进行交换，设置为放弃交换的流程
            Log.info(__FUNCTION__, "| set baseTask to abandon exchange");
            ProcessTask(*this, { "JieGarden@Roguelike@CoppersAbandonSwitch" }).run();

            return true;
        }

        Log.info(__FUNCTION__, "| exchanging copper: ", max_discard_it->name, " -> ", m_new_copper.name);
        swipe_to_the_right_of_copperlist(max_discard_it->col - 1);

        Point click_point(m_origin_x, m_origin_y + (max_discard_it->index - 1) * m_row_offset);
        ctrler()->click(click_point);

        // 完成交换操作后，设置为确认交换的流程
        Log.info(__FUNCTION__, "| set baseTask to confirm exchange");
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersTakeConfirm" }).run();
    } break;
    }

    clear();

    return true;
}

void asst::RoguelikeCoppersTaskPlugin::clear()
{
    m_copper_list.clear();
    m_new_copper = RoguelikeCopper();
    m_pending_copper.clear();
}

void asst::RoguelikeCoppersTaskPlugin::slowly_swipe_to_the_left_of_copperlist(int loop_times) const
{
    for (int i = 0; i < loop_times; ++i) {
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersListSlowlySwipeToTheLeft" }).run();
    }
}

void asst::RoguelikeCoppersTaskPlugin::slowly_swipe_to_the_right_of_copperlist(int loop_times) const
{
    for (int i = 0; i < loop_times; ++i) {
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersListSlowlySwipeToTheRight" }).run();
    }
}

void asst::RoguelikeCoppersTaskPlugin::swipe_to_the_left_of_copperlist(int loop_times) const
{
    for (int i = 0; i < loop_times; ++i) {
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersListSwipeToTheLeft" }).run();
    }
    ProcessTask(*this, { "SleepAfterOperListQuickSwipe" }).run();
}

void asst::RoguelikeCoppersTaskPlugin::swipe_to_the_right_of_copperlist(int loop_times) const
{
    for (int i = 0; i < loop_times; ++i) {
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersListSwipeToTheRight" }).run();
    }
    ProcessTask(*this, { "SleepAfterOperListQuickSwipe" }).run();
}
