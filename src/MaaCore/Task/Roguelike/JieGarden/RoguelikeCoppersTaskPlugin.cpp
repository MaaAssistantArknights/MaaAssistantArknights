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
        m_run_mode = CoppersTaskRunMode::EXCHANGE;
        Log.info(__FUNCTION__, "| plugin activated for EXCHANGE mode");
    }
    else if (task_name.ends_with("Roguelike@GetDropSwitch")) {
        m_run_mode = CoppersTaskRunMode::PICKUP;
        Log.info(__FUNCTION__, "| plugin activated for PICKUP mode");
    }
    else {
        return false;
    }

    const auto mode = m_config->get_mode();
    if (mode == RoguelikeMode::Investment) {
        return m_config->get_invest_with_more_score();
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
    case CoppersTaskRunMode::PICKUP:
        success = handle_pickup_mode();
        break;
    case CoppersTaskRunMode::EXCHANGE:
        success = handle_exchange_mode();
        break;
    }

    reset_in_run_variables();
    return success;
}

// 处理掉落通宝的拾取 识别交换按钮,roi偏移来识别通宝名称
bool asst::RoguelikeCoppersTaskPlugin::handle_pickup_mode()
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

    const auto name_task = Task.get<OcrTaskInfo>("JieGarden@Roguelike@CoppersAnalyzer-PickupNameOCR");
    RegionOCRer ocr(image);

    // 通过识别到的每个通宝类型来识别通宝名称
    for (size_t i = 0; i < match_results.size(); ++i) {
        const auto& [rect, score, templ_name] = match_results[i];

        Rect roi = rect.move(name_task->roi);
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
        auto copper_opt = create_copper_from_name(copper_name, 1, static_cast<int>(i), false, roi);
        if (!copper_opt) {
            Log.error(__FUNCTION__, "| failed to create copper at position", i);
            continue;
        }
        Point click_point(rect.x + rect.width / 2, rect.y + rect.height / 2);
        m_pending_copper.emplace_back(std::move(*copper_opt), click_point);

#ifdef ASST_DEBUG
        // 绘制识别出的通宝名称 - 不支持中文，所以只绘制score
        cv::rectangle(image_draw, cv::Rect(roi.x, roi.y, roi.width, roi.height), cv::Scalar(0, 255, 0), 2);
        cv::putText(
            image_draw,
            "score: " + std::to_string(ocr.get_result().score),
            cv::Point(roi.x, std::max(0, roi.y - 6)),
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
        const auto relative_path = relative_dir / (std::format("{}_pickup_draw.png", utils::format_now_for_filename()));
        Log.debug(__FUNCTION__, "| Saving pickup mode debug image to ", relative_path);
        asst::imwrite(relative_path, image_draw);
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, "| failed to save debug image:", e.what());
    }
#endif

    return true;
}

// 交换通宝 先识别通宝类型,然后roi偏移来OCR通宝名称和是否已投出
bool asst::RoguelikeCoppersTaskPlugin::handle_exchange_mode()
{
    swipe_copper_list_left(2); // 有时候进去不在最左边

    MultiMatcher matcher;
    RegionOCRer ocr(ctrler()->get_image());
    const auto name_task = Task.get<OcrTaskInfo>("JieGarden@Roguelike@CoppersAnalyzer-ExchangeNameOCR");
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
        if (match_results.empty()) {
            Log.error(__FUNCTION__, "| no coppers recognized in column", col);
            if (col == 0) {
                return false;
            }
            continue;
        }
        sort_by_vertical_(match_results);

        if (col > 0 && col < m_col) {
            m_origin_x = match_results.front().rect.x;
        }
        else if (col == m_col) {
            m_last_x = match_results.front().rect.x;
        }
        m_origin_y = match_results.front().rect.y;

        if (match_results.size() > 1) {
            m_row_offset = match_results[1].rect.y - match_results[0].rect.y;
        }

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

#ifdef ASST_DEBUG
            cv::rectangle(image_draw, cv::Rect(roi.x, roi.y, roi.width, roi.height), cv::Scalar(0, 0, 255), 2);
            cv::putText(
                image_draw,
                "score: " + std::to_string(ocr.get_result().score),
                cv::Point(roi.x, std::max(0, roi.y - 6)),
                cv::FONT_HERSHEY_SIMPLEX,
                0.45,
                cv::Scalar(0, 0, 255),
                1);
#endif

            if (col != 0) {
                // 识别是否已投出
                Rect cast_roi = match_result.rect.move(cast_task->roi);
                ocr.set_roi(cast_roi);
                if (ocr.analyze() && ocr.get_result().text.find("已投出") != std::string::npos) {
                    is_cast = true;
#ifdef ASST_DEBUG
                    cv::rectangle(
                        image_draw,
                        cv::Rect(cast_roi.x, cast_roi.y, cast_roi.width, cast_roi.height),
                        cv::Scalar(0, 0, 255),
                        2);
                    cv::putText(
                        image_draw,
                        "score: " + std::to_string(ocr.get_result().score),
                        cv::Point(cast_roi.x, std::max(0, cast_roi.y + cast_roi.height + 16)),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.45,
                        cv::Scalar(0, 0, 255),
                        1);
#endif
                }
            }

            Log.info(__FUNCTION__, "| found copper:", copper_name, "at (", col, ",", row, ")", "is_cast:", is_cast);

            auto copper_opt = create_copper_from_name(copper_name, col, static_cast<int>(row + 1), is_cast, roi);
            if (!copper_opt) {
                Log.error(__FUNCTION__, "| failed to create copper at (", col, ",", row, ")");
                continue;
            }

            auto copper = std::move(*copper_opt);
            copper.type = RoguelikeCoppersConfig::get_type_from_template(match_result.templ_name);

            if (col == 0) {
                m_new_copper = std::move(copper);
            }
            else {
                m_copper_list.emplace_back(std::move(copper));
            }
        }

        // 在中间列之间滑动
        if (col != 0 && col != m_col) {
            // col = 0 在识别左边新拾取的通宝
            // col = m_col 在识别最右边一列的通宝
            // 将列表向右滑动一列
            swipe_copper_list_right(1, true);
        }

#ifdef ASST_DEBUG
        try {
            const std::filesystem::path& relative_dir = utils::path("debug") / utils::path("roguelikeCoppers");
            const auto relative_path =
                relative_dir / (std::format("{}_exchange_draw.png", utils::format_now_for_filename()));
            Log.debug(__FUNCTION__, "| Saving exchange mode debug image to ", relative_path);
            asst::imwrite(relative_path, image_draw);
        }
        catch (const std::exception& e) {
            Log.error(__FUNCTION__, "| failed to save debug image:", e.what());
        }
#endif
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
            "handle_exchange_mode",
            "new copper (",
            m_new_copper.name,
            ") is worse than all existing coppers, abandoning exchange");
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersAbandonExchange" }).run();
        return true;
    }

    Log.info(
        __FUNCTION__,
        "| exchanging copper:",
        worst_it->name,
        "(",
        worst_it->col,
        ",",
        worst_it->row,
        ")",
        "->",
        m_new_copper.name);

    // 点击通宝后执行确认交换任务

    click_copper_at_position(worst_it->col, worst_it->row);

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

void asst::RoguelikeCoppersTaskPlugin::click_copper_at_position(int col, int row) const
{
    int x = col == m_col ? m_last_x : m_origin_x;
    Point click_point(x, m_origin_y + (row - 1) * m_row_offset);

    Log.debug(
        __FUNCTION__,
        "| clicking copper at (",
        col,
        ",",
        row,
        ") -> point (",
        click_point.x,
        ",",
        m_origin_y,
        "+",
        (row - 1),
        "*",
        m_row_offset,
        ")");

    // 滑动回到最左边
    swipe_copper_list_left(m_col);
    sleep(300);

    swipe_copper_list_right(col - 1, true);

    ctrler()->click(click_point);
    sleep(300);
}

std::optional<asst::RoguelikeCopper> asst::RoguelikeCoppersTaskPlugin::create_copper_from_name(
    const std::string& name,
    int col,
    int row,
    bool is_cast,
    const Rect& pos) const
{
    if (auto found_copper = RoguelikeCoppers.find_copper(m_config->get_theme(), name)) {
        auto copper = *found_copper;
        copper.col = col;
        copper.row = row;
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
        return copper;
    }

    Log.error(__FUNCTION__, "| copper not found in config:", name);

    try {
        cv::Mat screen = ctrler()->get_image();
        if (!screen.empty()) {
            cv::Mat screen_draw = screen.clone();
            cv::rectangle(screen_draw, cv::Rect(pos.x, pos.y, pos.width, pos.height), cv::Scalar(0, 0, 255), 2);
            const std::filesystem::path& relative_dir = utils::path("debug") / utils::path("roguelike");
            const auto relative_path =
                relative_dir / (std::format("{}_unknown_draw.png", utils::format_now_for_filename()));
            Log.debug(__FUNCTION__, "| Saving unknown copper debug image to", relative_path);
            asst::imwrite(relative_path, screen_draw);
        }
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, "| failed to save unknown copper debug image:", e.what());
    }

    return std::nullopt;
}
