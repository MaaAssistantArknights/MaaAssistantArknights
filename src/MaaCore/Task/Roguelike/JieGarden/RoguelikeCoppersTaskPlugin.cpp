#include "RoguelikeCoppersTaskPlugin.h"
#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
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
    m_column_offset = config->special_params.at(4);
    m_row_offset = config->special_params.at(5);
    m_ocr_roi_offset_y = config->special_params.at(6);

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

    const std::string task_name = details.get("details", "task", "");
    if (task_name.ends_with("Roguelike@CoppersTakeFlag")) {
        m_run_mode = CoppersTaskRunMode::SWITCH;
        Log.info(__FUNCTION__, "| plugin activated for SWITCH mode");
    }
    else if (task_name.ends_with("Roguelike@GetDropSwitch")) {
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
    m_pending_copper.clear();
}

bool asst::RoguelikeCoppersTaskPlugin::_run()
{
    LogTraceFunction;

    bool success = false;
    switch (m_run_mode) {
    case CoppersTaskRunMode::CHOOSE:
        success = handle_choose_mode();
        break;
    case CoppersTaskRunMode::SWITCH:
        success = handle_switch_mode();
        break;
    }

    reset_in_run_variables();
    return success;
}

// 处理掉落通宝的拾取 识别交换按钮，roi偏移来识别通宝名称
bool asst::RoguelikeCoppersTaskPlugin::handle_choose_mode()
{
    MultiMatcher matcher;
    matcher.set_task_info("JieGarden@Roguelike@GetDropSwitch");
    auto image = ctrler()->get_image();
    matcher.set_image(image);

#ifdef ASST_DEBUG
    cv::Mat image_draw = image.clone();
#endif

    if (!matcher.analyze()) {
        Log.error(__FUNCTION__, "| failed to analyze GetDropSwitch");
        return false;
    }

    auto match_results = matcher.get_result();
    sort_by_horizontal_(match_results);

    const auto name_task = Task.get<OcrTaskInfo>("JieGarden@Roguelike@CoppersAnalyzer-NameOCR");
    RegionOCRer ocr(image);

    // 通过识别到的每个通宝类型来识别通宝名称
    for (size_t i = 0; i < match_results.size(); ++i) {
        const auto& [rect, score, templ_name] = match_results[i];

        Rect roi(rect.x, rect.y + m_ocr_roi_offset_y, rect.width, rect.height);
        ocr.set_roi(roi);
        ocr.set_replace(name_task->replace_map, name_task->replace_full);

        if (!ocr.analyze()) {
            Log.error(__FUNCTION__, "| failed to recognize copper name at position", i);
            continue;
        }

        const std::string copper_name = ocr.get_result().text;
        if (copper_name.empty()) {
            Log.error(__FUNCTION__, "| empty copper name at position", i);
            continue;
        }

        Log.info(__FUNCTION__, "| found copper:", copper_name, "at position", i);

        auto copper = create_copper_from_name(copper_name, 1, static_cast<int>(i));
        Point click_point(rect.x + rect.width / 2, rect.y + rect.height / 2);
        m_pending_copper.emplace_back(std::move(copper), click_point);

#ifdef ASST_DEBUG
        // 绘制识别出的通宝名称 - 不支持中文，所以只绘制score
        cv::rectangle(
            image_draw,
            cv::Rect(rect.x, rect.y + m_ocr_roi_offset_y, rect.width, rect.height),
            cv::Scalar(0, 255, 0),
            2);
        cv::putText(
            image_draw,
            "score: " + std::to_string(ocr.get_result().score),
            cv::Point(rect.x, std::max(0, rect.y + m_ocr_roi_offset_y - 6)),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 255, 0),
            1);
#endif
    }

    if (m_pending_copper.empty()) {
        Log.error(__FUNCTION__, "| no valid coppers found for pickup");
        return false;
    }

    // 寻找 m_pending_copper 中 pickup_priority 最大的
    auto max_pickup_it =
        std::max_element(m_pending_copper.begin(), m_pending_copper.end(), [](const auto& a, const auto& b) {
            return a.first.pickup_priority < b.first.pickup_priority;
        });

    Log.info(
        __FUNCTION__,
        "| selecting copper:",
        max_pickup_it->first.name,
        "with priority:",
        max_pickup_it->first.pickup_priority);
    ctrler()->click(max_pickup_it->second);

#ifdef ASST_DEBUG
    try {
        const std::filesystem::path& relative_dir = utils::path("debug") / utils::path("roguelikeCoppers");
        const auto relative_path = relative_dir / (utils::get_time_filestem() + "_choose_draw.png");
        Log.debug(__FUNCTION__, "| Saving choose mode debug image to ", relative_path);
        asst::imwrite(relative_path, image_draw);
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, "| failed to save debug image:", e.what());
    }
#endif

    return true;
}

// 交换通宝 先识别通宝类型，然后roi偏移来OCR通宝名称和是否已投出
bool asst::RoguelikeCoppersTaskPlugin::handle_switch_mode()
{
    swipe_copper_list_left(2); // 有时候进去不在最左边

    MultiMatcher matcher;
    RegionOCRer ocr(ctrler()->get_image());
    const auto name_task = Task.get<OcrTaskInfo>("JieGarden@Roguelike@CoppersAnalyzer-NameOCR");
    const auto cast_task = Task.get<OcrTaskInfo>("JieGarden@Roguelike@CoppersAnalyzer-CastOCR");

#ifdef ASST_DEBUG
    cv::Mat image_draw;
#endif

    // 扫描所有列的通宝
    for (int col = 0; col <= m_col; ++col) {
        // 根据列位置设置匹配任务
        if (col == 0) {
            matcher.set_task_info("JieGarden@Roguelike@CoppersAnalyzer-LeftType");
        }
        else if (col == m_col) {
            matcher.set_task_info("JieGarden@Roguelike@CoppersAnalyzer-RightType");
        }
        else {
            matcher.set_task_info("JieGarden@Roguelike@CoppersAnalyzer-Type");
        }

        auto image = ctrler()->get_image();
#ifdef ASST_DEBUG
        image_draw = image.clone();
#endif
        matcher.set_image(image);
        ocr.set_image(image);

        if (!matcher.analyze()) {
            Log.error(__FUNCTION__, "| no coppers recognized in column", col);
            if (col == 0) {
                return false;
            }
            continue;
        }

        auto match_results = matcher.get_result();
        sort_by_vertical_(match_results);

        // 处理当前列的匹配结果
        for (size_t row = 0; row < match_results.size(); ++row) {
            bool is_cast = false;

            const auto& match_result = match_results[row];

            Rect roi = match_result.rect.move(name_task->roi);
            ocr.set_roi(roi);
            ocr.set_replace(name_task->replace_map, name_task->replace_full);

            if (!ocr.analyze()) {
                Log.error(__FUNCTION__, "| failed to recognize copper name at (", col, ",", row, ")");
                continue;
            }

            const std::string copper_name = ocr.get_result().text;
            if (copper_name.empty()) {
                Log.error(__FUNCTION__, "| empty copper name at (", col, ",", row, ")");
                continue;
            }

            if (col != 0) {
                // 识别是否已投出
                Rect cast_roi = match_result.rect.move(cast_task->roi);
                ocr.set_roi(cast_roi);
                if (ocr.analyze() && ocr.get_result().text.find("已投出") != std::string::npos) {
                    is_cast = true;
                }
            }

            Log.info(__FUNCTION__, "| found copper:", copper_name, "at (", col, ",", row, ")", "is_cast:", is_cast);

            auto copper = create_copper_from_name(copper_name, col, static_cast<int>(row + 1));
            copper.type = RoguelikeCoppersConfig::get_type_from_template(match_result.templ_name);

            if (col == 0) {
                m_new_copper = std::move(copper);
            }
            else {
                m_copper_list.emplace_back(std::move(copper));
            }

#ifdef ASST_DEBUG
            cv::rectangle(image_draw, cv::Rect(roi.x, roi.y, roi.width, roi.height), cv::Scalar(0, 0, 255), 2);
            // cv::putText(
            //     image_draw,
            //     copper_name,
            //     cv::Point(match_result.rect.x, std::max(0, match_result.rect.y - 6)),
            //     cv::FONT_HERSHEY_SIMPLEX,
            //     0.45,
            //     cv::Scalar(0, 0, 255),
            //     1);
            try {
                const std::filesystem::path& relative_dir = utils::path("debug") / utils::path("roguelikeCoppers");
                const auto relative_path = relative_dir / (utils::get_time_filestem() + "_switch_draw.png");
                Log.debug(__FUNCTION__, "| Saving switch mode debug image to ", relative_path);
                asst::imwrite(relative_path, image_draw);
            }
            catch (const std::exception& e) {
                Log.error(__FUNCTION__, "| failed to save debug image:", e.what());
            }
#endif
        }

        // 在中间列之间滑动
        if (col != 0 && col != m_col) {
            // col = 0 在识别左边新拾取的通宝
            // col = m_col 在识别最右边一列的通宝
            // 将列表向右滑动一列
            swipe_copper_list_right(1, true);
        }
    }

    if (m_copper_list.empty()) {
        Log.error(__FUNCTION__, "| no coppers found in list for comparison");
        return false;
    }

    // 决定是否交换并执行
    auto worst_it = std::max_element(m_copper_list.begin(), m_copper_list.end(), [](const auto& a, const auto& b) {
        return a.get_copper_discard_priority() < b.get_copper_discard_priority();
    });

    if (worst_it->get_copper_discard_priority() < m_new_copper.get_copper_discard_priority()) {
        Log.info(
            "handle_switch_mode",
            "new copper (",
            m_new_copper.name,
            ") is worse than all existing coppers, abandoning exchange");
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersAbandonSwitch" }).run();
        return true;
    }

    Log.info(
        __FUNCTION__,
        "| exchanging copper:",
        worst_it->name,
        "(",
        worst_it->col,
        ",",
        worst_it->index,
        ")",
        "->",
        m_new_copper.name);

    // 点击通宝后执行确认交换任务

    click_copper_at_position(worst_it->index);

    ProcessTask(*this, { "JieGarden@Roguelike@CoppersTakeConfirm" }).run();

    return true;
}

void asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_left(int times, bool slowly) const
{
    for (int i = 0; i < times; ++i) {
        std::string task_name = slowly ? "JieGarden@Roguelike@CoppersListSlowlySwipeToTheLeft"
                                       : "JieGarden@Roguelike@CoppersListSwipeToTheLeft";

        ProcessTask(*this, { task_name }).run();
    }
}

void asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_right(int times, bool slowly) const
{
    for (int i = 0; i < times; ++i) {
        std::string task_name = slowly ? "JieGarden@Roguelike@CoppersListSlowlySwipeToTheRight"
                                       : "JieGarden@Roguelike@CoppersListSwipeToTheRight";

        ProcessTask(*this, { task_name }).run();
    }
}

void asst::RoguelikeCoppersTaskPlugin::click_copper_at_position(int index) const
{
    int col = index / m_row;
    int row = index % m_row;
    Point click_point(m_origin_x, m_origin_y + (row - 1) * m_row_offset);

    // 滑动回到最左边
    swipe_copper_list_left(m_col);
    sleep(300);

    swipe_copper_list_right(col - 1);

    ctrler()->click(click_point);
    sleep(300);
}

asst::RoguelikeCopper
    asst::RoguelikeCoppersTaskPlugin::create_copper_from_name(const std::string& name, int col, int index, bool is_cast)
        const
{
    RoguelikeCopper copper;

    if (auto found_copper = RoguelikeCoppers.find_copper(m_config->get_theme(), name)) {
        copper = *found_copper;
        copper.col = col;
        copper.index = index;
        copper.is_cast = is_cast;
        Log.info(
            __FUNCTION__,
            "| created copper:",
            name,
            "priority:",
            copper.pickup_priority,
            "/",
            copper.discard_priority,
            "/",
            copper.cast_discard_priority);
    }
    else {
        Log.error(__FUNCTION__, "| copper not found in config:", name, "type:", static_cast<int>(copper.type));

        try {
            cv::Mat screen = ctrler()->get_image();
            if (!screen.empty()) {
                cv::Mat screen_draw = screen.clone();
                // const std::string label = "Unknown copper: " + name;
                // cv::putText(
                //     screen_draw,
                //     label,
                //     cv::Point(10, 30),
                //     cv::FONT_HERSHEY_SIMPLEX,
                //     0.8,
                //     cv::Scalar(0, 0, 255),
                //     2);
                const std::filesystem::path& relative_dir = utils::path("debug") / utils::path("roguelikeCoppers");
                const auto relative_path = relative_dir / (utils::get_time_filestem() + "_unknown_copper.png");
                Log.debug(__FUNCTION__, "| Saving unknown copper debug image to ", relative_path);
                asst::imwrite(relative_path, screen_draw);
            }
        }
        catch (const std::exception& e) {
            Log.error(__FUNCTION__, "| failed to save unknown copper debug image:", e.what());
        }
    }

    return copper;
}
