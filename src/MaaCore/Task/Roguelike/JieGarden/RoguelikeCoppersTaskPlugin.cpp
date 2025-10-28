#include "RoguelikeCoppersTaskPlugin.h"
#include "Common/AsstTypes.h"
#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Roguelike/JieGarden/RoguelikeCoppersAnalyzer.h"

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
    auto image = ctrler()->get_image();

#ifdef ASST_DEBUG
    cv::Mat image_draw = image.clone();
#endif

    RoguelikeCoppersAnalyzer analyzer(image);
    if (!analyzer.analyze_pickup()) {
        Log.error(__FUNCTION__, "| failed to analyze GetDropSwitch");
        return false;
    }

    const auto& detections = analyzer.get_detections();
    if (detections.empty()) {
        Log.error(__FUNCTION__, "| no detections returned for pickup mode");
        return false;
    }

    // 通过识别到的每个通宝类型来识别通宝名称
    for (size_t i = 0; i < detections.size(); ++i) {
        const auto& detection = detections[i];

        Log.info(__FUNCTION__, std::format("| found copper: {} at position {}", detection.name, i));
        auto copper_opt = create_copper_from_name(detection.name, 1, static_cast<int>(i), false, detection.name_roi);
        if (!copper_opt) {
            Log.error(__FUNCTION__, std::format("| failed to create copper at position {}", i));
            continue;
        }

        m_pending_copper.emplace_back(std::move(*copper_opt), detection.click_point);

#ifdef ASST_DEBUG
        draw_detection_debug(image_draw, detection, cv::Scalar(0, 255, 0));
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
        std::format(
            "| selecting copper: {} with priority: {}",
            max_pickup_it->first.name,
            max_pickup_it->first.pickup_priority));
    ctrler()->click(max_pickup_it->second);

#ifdef ASST_DEBUG
    save_debug_image(image_draw, "pickup");
#endif

    return true;
}

// 交换通宝 先识别通宝类型,然后roi偏移来OCR通宝名称和是否已投出
// 图片示意请看 文档(docs\zh-cn\protocol\integrated-strategy-schema.md) 或 #13835
bool asst::RoguelikeCoppersTaskPlugin::handle_exchange_mode()
{
    bool ret = swipe_copper_list_left(2); // 有时候进去不在最左边

    // 拆出 col == 0时的情况
    // -----------------------------------------------------
    // 识别左侧新拾取的通宝
    auto image = ctrler()->get_image();

#ifdef ASST_DEBUG
    cv::Mat image_draw = image.clone();
#endif
    RoguelikeCoppersAnalyzer left_analyzer(image);
    if (!left_analyzer.analyze_column(RoguelikeCoppersAnalyzer::ColumnRole::Leftmost, false)) {
        Log.error(__FUNCTION__, "| no coppers recognized in column 0");
        return false;
    }

    const auto& left_detections = left_analyzer.get_detections();
    if (left_detections.empty()) {
        Log.error(__FUNCTION__, "| no coppers recognized in column 0");
        return false;
    }

    // 处理当前列的匹配结果
    for (size_t row = 0; row < left_detections.size(); ++row) {
        const auto& detection = left_detections[row];

        Log.info(__FUNCTION__, std::format("| found copper: {} at ({},{}) is_cast: {}", detection.name, 0, row, false));

#ifdef ASST_DEBUG
        draw_detection_debug(image_draw, detection, cv::Scalar(0, 0, 255));
#endif

        auto copper_opt =
            create_copper_from_name(detection.name, 0, static_cast<int>(row + 1), false, detection.name_roi);
        if (!copper_opt) {
            Log.error(__FUNCTION__, std::format("| failed to create copper at ({},{})", 0, row));
            continue;
        }

        auto copper = std::move(*copper_opt);
        copper.type = RoguelikeCoppersConfig::get_type_from_template(detection.templ_name);

        m_new_copper = std::move(copper);
    }

    // -----------------------------------------------------
    auto image_last = ctrler()->get_image();

    // 扫描所有列的通宝
    for (int col = 1; col <= 999; ++col) { // 总不可能超过999列(2997个)通宝吧
        // 识别上次点击的右侧的通宝是否被滑动到中间，识别到了则说明被滑动到中间，即不是最后一列。
        bool is_last_col =
            col == 1
                ? false
                : !ProcessTask(*this, { "JieGarden@Roguelike@CoppersAnalyzer-TypeSelected" }).set_retry_times(2).run();

        // 点击右侧上方的通宝作为滑动成功的标注
        if (!is_last_col) {
            ret &= ProcessTask(*this, { "JieGarden@Roguelike@CoppersListSwipeFlagClick" }).run();
        }

        image = ctrler()->get_image();
        if (image_last.data == image.data) {
            Log.error(__FUNCTION__, std::format("| image not updated after swipe at column {}", col));
            break;
        }
        image_last = image;
#ifdef ASST_DEBUG
        image_draw = image.clone();
#endif
        RoguelikeCoppersAnalyzer::ColumnRole role = is_last_col ? RoguelikeCoppersAnalyzer::ColumnRole::Rightmost
                                                                : RoguelikeCoppersAnalyzer::ColumnRole::Middle;
        RoguelikeCoppersAnalyzer column_analyzer(image);
        if (!column_analyzer.analyze_column(role, true)) {
            Log.error(__FUNCTION__, std::format("| no coppers recognized in column {}", col));
            continue;
        }

        const auto& detections = column_analyzer.get_detections();
        if (detections.empty()) {
            Log.error(__FUNCTION__, std::format("| no coppers recognized in column {}", col));
            continue;
        }

        const auto& metrics = column_analyzer.get_column_metrics();

        // 根据列类型更新坐标基准点
        update_column_coordinates(metrics, is_last_col);

        // 处理当前列的匹配结果
        for (size_t row = 0; row < detections.size(); ++row) {
            const auto& detection = detections[row];

            Log.info(
                __FUNCTION__,
                std::format("| found copper: {} at ({},{}) is_cast: {}", detection.name, col, row, detection.is_cast));

#ifdef ASST_DEBUG
            draw_detection_debug(image_draw, detection, cv::Scalar(0, 0, 255));
#endif

            auto copper_opt = create_copper_from_name(
                detection.name,
                col,
                static_cast<int>(row + 1),
                detection.is_cast,
                detection.name_roi);
            if (!copper_opt) {
                Log.error(__FUNCTION__, std::format("| failed to create copper at ({},{})", col, row));
                continue;
            }

            auto copper = std::move(*copper_opt);
            copper.type = RoguelikeCoppersConfig::get_type_from_template(detection.templ_name);

            m_copper_list.emplace_back(std::move(copper));
        }

        // 在中间列之间滑动
        if (!is_last_col) {
            // 将列表向右滑动一列
            swipe_copper_list_right(1, true);
        }

#ifdef ASST_DEBUG
        save_debug_image(image_draw, "exchange");
#endif
        if (is_last_col) {
            m_col = col;
            Log.info(__FUNCTION__, std::format("| total columns detected: {}", m_col));
            break;
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
            __FUNCTION__,
            std::format("new copper ({}) is worse than all existing coppers, abandoning exchange", m_new_copper.name));
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersAbandonExchange" }).run();
        return true;
    }

    Log.info(
        __FUNCTION__,
        std::format(
            "| exchanging copper: {} ({},{}) -> {}",
            worst_it->name,
            worst_it->col,
            worst_it->row,
            m_new_copper.name));

    // 发送通宝替换信息到 WPF
    auto copper_info = basic_info_with_what("RoguelikeCoppersExchangeInfo");
    copper_info["details"]["to_discard"] = std::format("{} ({},{})", worst_it->name, worst_it->col, worst_it->row);
    copper_info["details"]["to_pickup"] = m_new_copper.name;
    callback(AsstMsg::SubTaskExtraInfo, copper_info);

    // 点击通宝后执行确认交换任务
    click_copper_at_position(worst_it->col, worst_it->row);

    ret &= ProcessTask(*this, { "JieGarden@Roguelike@CoppersTakeConfirm" }).run();

    return ret;
}

bool asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_left(int times, bool slowly) const
{
    bool ret = true;
    for (int i = 0; i < times; ++i) {
        std::string task_name = slowly ? "JieGarden@Roguelike@CoppersListSlowlySwipeToTheLeft"
                                       : "JieGarden@Roguelike@CoppersListSwipeToTheLeft";

        ret &= ProcessTask(*this, { task_name }).run();
    }
    return ret;
}

bool asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_right(int times, bool slowly) const
{
    bool ret = true;
    for (int i = 0; i < times; ++i) {
        std::string task_name = slowly ? "JieGarden@Roguelike@CoppersListSlowlySwipeToTheRight"
                                       : "JieGarden@Roguelike@CoppersListSwipeToTheRight";

        ret &= ProcessTask(*this, { task_name }).run();
    }
    return ret;
}

void asst::RoguelikeCoppersTaskPlugin::click_copper_at_position(int col, int row) const
{
    int x = col == m_col ? m_last_x : m_origin_x;
    Point click_point(x, m_origin_y + (row - 1) * m_row_offset);

    Log.debug(
        __FUNCTION__,
        std::format(
            "| clicking copper at ({},{}) -> point ({},{},{},{})",
            col,
            row,
            click_point.x,
            m_origin_y,
            (row - 1),
            m_row_offset));

    // 滑动回到最左边
    swipe_copper_list_left(m_col);
    sleep(300);

    swipe_copper_list_right(col - 1, true);

    ctrler()->click(click_point);
    sleep(300);
}

// 辅助函数：根据列类型更新坐标基准点
void asst::RoguelikeCoppersTaskPlugin::update_column_coordinates(
    const RoguelikeCoppersAnalyzer::ColumnMetrics& metrics,
    bool is_last_col)
{
    // 根据列类型更新X坐标基准点：非最后一列使用origin_x，最后一列记录last_x用于点击计算
    m_origin_x = !is_last_col ? metrics.origin_x : m_origin_x;
    m_last_x = is_last_col ? metrics.origin_x : m_last_x;

    // 更新统一的Y坐标基准点和行偏移量
    m_origin_y = metrics.origin_y;
    if (metrics.row_offset != 0) {
        m_row_offset = metrics.row_offset;
    }
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
            std::format(
                "| created copper: {} priority: {}/{}/{}",
                name,
                copper.pickup_priority,
                copper.discard_priority,
                copper.cast_discard_priority));
        return copper;
    }

    Log.error(__FUNCTION__, std::format("| copper not found in config: {}", name));

    try {
        cv::Mat screen = ctrler()->get_image();
        if (!screen.empty()) {
            cv::Mat screen_draw = screen.clone();
            const static std::vector<int> jpeg_params = { cv::IMWRITE_JPEG_QUALITY, 95, cv::IMWRITE_JPEG_OPTIMIZE, 1 };
            cv::rectangle(screen_draw, cv::Rect(pos.x, pos.y, pos.width, pos.height), cv::Scalar(0, 0, 255), 2);
            const std::filesystem::path& relative_dir = utils::path("debug") / utils::path("roguelike");
            const auto relative_path =
                relative_dir / (std::format("{}_unknown_draw.jpeg", utils::format_now_for_filename()));
            Log.debug(__FUNCTION__, "| Saving unknown copper debug image to", relative_path);
            asst::imwrite(relative_path, screen_draw, jpeg_params);
        }
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, std::format("| failed to save unknown copper debug image: {}", e.what()));
    }

    return std::nullopt;
}

#ifdef ASST_DEBUG
// 调试绘制辅助函数实现
void asst::RoguelikeCoppersTaskPlugin::draw_detection_debug(
    cv::Mat& image,
    const RoguelikeCoppersAnalyzer::CopperDetection& detection,
    const cv::Scalar& color) const
{
    // 绘制名称识别区域
    cv::rectangle(
        image,
        cv::Rect(detection.name_roi.x, detection.name_roi.y, detection.name_roi.width, detection.name_roi.height),
        color,
        2);
    cv::putText(
        image,
        std::format("score: {:.6f}", detection.name_score),
        cv::Point(detection.name_roi.x, std::max(0, detection.name_roi.y - 6)),
        cv::FONT_HERSHEY_SIMPLEX,
        0.45,
        color,
        1);

    // 如果有已投出状态识别，绘制已投出区域
    if (detection.cast_recognized) {
        cv::rectangle(
            image,
            cv::Rect(detection.cast_roi.x, detection.cast_roi.y, detection.cast_roi.width, detection.cast_roi.height),
            color,
            2);
        cv::putText(
            image,
            std::format("cast_score: {:.6f}", detection.cast_score),
            cv::Point(detection.cast_roi.x, std::max(0, detection.cast_roi.y + detection.cast_roi.height + 16)),
            cv::FONT_HERSHEY_SIMPLEX,
            0.45,
            color,
            1);
    }
}

void asst::RoguelikeCoppersTaskPlugin::save_debug_image(const cv::Mat& image, const std::string& suffix) const
{
    try {
        const static std::vector<int> jpeg_params = { cv::IMWRITE_JPEG_QUALITY, 95, cv::IMWRITE_JPEG_OPTIMIZE, 1 };
        const std::filesystem::path& relative_dir = utils::path("debug") / utils::path("roguelikeCoppers");
        const auto relative_path =
            relative_dir / (std::format("{}_{}_draw.jpeg", utils::format_now_for_filename(), suffix));
        Log.debug(__FUNCTION__, "| Saving debug image to ", relative_path);
        asst::imwrite(relative_path, image, jpeg_params);
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, "| failed to save debug image:", e.what());
    }
}
#endif
