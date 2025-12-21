#include "RoguelikeCoppersTaskPlugin.h"

#include "Common/AsstTypes.h"
#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/ImageIo.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/RoguelikeConfig.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
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
    // 只处理子任务开始消息且为ProcessTask类型
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& theme = m_config->get_theme();
    if (theme != RoguelikeTheme::JieGarden) {
        return false;
    }

    const std::string task_name = details.get("details", "task", "");

    // 根据任务名称确定运行模式
    if (task_name.ends_with("Roguelike@CoppersTakeFlag")) {
        m_run_mode = CoppersTaskRunMode::EXCHANGE;
        Log.info(__FUNCTION__, "| plugin activated for EXCHANGE mode");
    }
    else if (task_name.ends_with("Roguelike@GetDropSwitch")) {
        m_run_mode = CoppersTaskRunMode::PICKUP;
        Log.info(__FUNCTION__, "| plugin activated for PICKUP mode");
    }
    else {
        return false; // 不支持的任务类型
    }

    // 投资模式下需要额外检查是否启用购物功能
    const auto mode = m_config->get_mode();
    if (mode == RoguelikeMode::Investment) {
        return m_config->get_invest_with_more_score();
    }

    return true;
}

// 重置运行时变量，为新一轮执行做准备
void asst::RoguelikeCoppersTaskPlugin::reset_in_run_variables()
{
    m_copper_list.clear();
    m_new_copper = RoguelikeCopper();
    m_pending_copper.clear();

    // 重置坐标计算相关变量
    m_col = 0;
    m_origin_x = 0;
    m_x = 0;
    m_last_x = 0;
    m_y = 0;
    m_row_offset = 0;
}

// 执行插件主要逻辑，根据当前运行模式处理通宝拾取或交换
bool asst::RoguelikeCoppersTaskPlugin::_run()
{
    LogTraceFunction;

    CopperTaskResult result = CopperTaskResult::FAILED;
    // 根据运行模式调用相应的处理函数
    switch (m_run_mode) {
    case CoppersTaskRunMode::PICKUP:
        result = handle_pickup_mode();
        break;
    case CoppersTaskRunMode::EXCHANGE:
        result = handle_exchange_mode();
        break;
    }

    // 执行完成后重置变量
    reset_in_run_variables();

    // 将结果转换为bool：SUCCESS和SKIPPED都返回true，FAILED返回false
    return (result == CopperTaskResult::SUCCESS || result == CopperTaskResult::SKIPPED);
}

// 处理掉落通宝的拾取：识别交换按钮，ROI偏移来识别通宝名称
asst::CopperTaskResult asst::RoguelikeCoppersTaskPlugin::handle_pickup_mode()
{
    LogTraceFunction;

    const auto& image = ctrler()->get_image();

#ifdef ASST_DEBUG
    cv::Mat image_draw = image.clone();
#endif

    // 使用Analyzer识别拾取界面中的通宝
    RoguelikeCoppersAnalyzer analyzer(image);
    if (!analyzer.analyze_pickup()) {
        LogError << __FUNCTION__ << "| no coppers recognized for pickup";
        return CopperTaskResult::FAILED;
    }

    const auto& detections = analyzer.get_detections();
    if (detections.empty()) {
        LogError << __FUNCTION__ << "| no detections returned for pickup mode";
        return CopperTaskResult::FAILED;
    }

    asst::Point click_point_fallback(0, 0);

    // 遍历每个检测到的通宝，创建通宝对象
    for (size_t i = 0; i < detections.size(); ++i) {
        const auto& detection = detections[i];

        LogInfo << __FUNCTION__ << "| found copper:" << detection.name << "at position" << i;

        // 根据识别到的名称创建通宝对象
        auto copper_opt = create_copper_from_name(detection.name, 1, static_cast<int>(i), false, detection.name_roi);
        if (!copper_opt) {
            LogError << __FUNCTION__ << "| failed to create copper at position" << i << " name:" << detection.name;

            click_point_fallback = detection.click_point;
            continue;
        }

        // 将通宝及其点击坐标保存到待选列表
        m_pending_copper.emplace_back(std::move(*copper_opt), detection.click_point);

#ifdef ASST_DEBUG
        // 调试模式下在图像上绘制检测结果（绿色表示拾取模式）
        draw_detection_debug(image_draw, detection, cv::Scalar(0, 255, 0));
#endif
    }

    if (m_pending_copper.empty()) {
        LogError << __FUNCTION__ << "| no valid coppers found for pickup";
        // 如果没有有效通宝，尝试点击最后一个检测到的名称错误的通宝位置作为回退
        if (click_point_fallback.x != 0 && click_point_fallback.y != 0) {
            LogWarn << __FUNCTION__ << "| clicking fallback point at (" << click_point_fallback.x << ","
                    << click_point_fallback.y << ")";
            ctrler()->click(click_point_fallback);
            return CopperTaskResult::SKIPPED;
        }
        LogError << __FUNCTION__ << "| no coppers recognized to fallback";
        return CopperTaskResult::FAILED;
    }

    auto strategy = get_copper_strategy(get_current_strategy_type());

    std::vector<RoguelikeCopper> options;
    for (const auto& [copper, point] : m_pending_copper) {
        options.push_back(copper);
    }

    auto result = strategy.pickup(options, m_copper_list);
    if (result.selected_index >= m_pending_copper.size()) {
        LogError << __FUNCTION__ << "| invalid pickup index: " << result.selected_index;
        return CopperTaskResult::FAILED;
    }

    const auto& [copper, point] = m_pending_copper[result.selected_index];
    LogInfo << __FUNCTION__ << "| selected" << copper.name << "reason:" << result.reason;

    ctrler()->click(point);

#ifdef ASST_DEBUG
    // 保存调试图像
    save_debug_image(image_draw, "pickup");
#endif

    return CopperTaskResult::SUCCESS;
}

// 交换通宝：先识别通宝类型，然后ROI偏移来OCR通宝名称和是否已投出
// 图片示意请看 文档(docs\zh-cn\protocol\integrated-strategy-schema.md) 或 #13835
asst::CopperTaskResult asst::RoguelikeCoppersTaskPlugin::handle_exchange_mode()
{
    // 确保列表滑动到最左边（有时候进入界面不在最左边）
    bool ret = swipe_copper_list_to_leftmost(2);

    // =================================================
    // 第一步：识别左侧新拾取的通宝（col == 0）
    // =================================================
    auto image = ctrler()->get_image();

#ifdef ASST_DEBUG
    cv::Mat image_draw = image.clone();
#endif

    // 分析左侧新拾取通宝列（不检测已投出状态）
    RoguelikeCoppersAnalyzer left_analyzer(image);
    if (!left_analyzer.analyze_column(RoguelikeCoppersAnalyzer::ColumnRole::Leftmost, false)) {
        LogInfo << __FUNCTION__ << "| left column copper recognition failed, skipping exchange";
        // 直接进入next，放弃交换
        return CopperTaskResult::SKIPPED;
    }

    const auto& left_detections = left_analyzer.get_detections();
    if (left_detections.empty()) {
        LogInfo << __FUNCTION__ << "| left column is empty, skipping exchange";
        // 直接进入next，放弃交换
        return CopperTaskResult::SKIPPED;
    }

    // 处理左侧列的检测结果
    if (left_detections.size() != 1) {
        LogWarn << __FUNCTION__ << "| expected exactly one copper in left column, got" << left_detections.size()
                << ", skipping exchange";

#ifdef ASST_DEBUG
        for (const auto& detection : left_detections) {
            draw_detection_debug(image_draw, detection, cv::Scalar(0, 0, 255));
        }
        save_debug_image(image_draw, "left_column_unexpected_count");
#endif
        // 直接进入next，放弃交换
        return CopperTaskResult::SKIPPED;
    }

    {
        const auto& detection = left_detections[0];

        LogInfo << __FUNCTION__ << "| found copper:" << detection.name << "at (0,0)";

#ifdef ASST_DEBUG
        // 调试模式下绘制检测结果（红色表示新拾取的通宝）
        draw_detection_debug(image_draw, detection, cv::Scalar(0, 0, 255));
#endif

        // 创建新拾取的通宝对象
        auto copper_opt = create_copper_from_name(detection.name, 0, 0, false, detection.name_roi);
        if (!copper_opt) {
            LogError << __FUNCTION__ << "| failed to create copper at (0,0)";
            // 直接进入next，放弃交换
            return CopperTaskResult::SKIPPED;
        }

        auto copper = std::move(*copper_opt);
        // 根据模板确定通宝类型
        copper.type = RoguelikeCoppersConfig::get_type_from_template(detection.templ_name);

        m_new_copper = std::move(copper);

#ifdef ASST_DEBUG
        save_debug_image(image_draw, "new_copper");
#endif
    }

    // =================================================
    // 第二步：扫描所有现有通宝列
    // =================================================
    auto image_last = ctrler()->get_image();

    // 预分配10个通宝的空间
    m_copper_list.reserve(10);

    // 总不可能超过20列(60个)通宝吧
    for (int col = 1; col <= 20; ++col) {
        // 检查是否是最后一列：尝试识别右侧通宝是否滑动到中间位置
        bool is_last_col =
            col == 1
                ? false
                : !ProcessTask(*this, { "JieGarden@Roguelike@CoppersAnalyzer-TypeSelected" }).set_retry_times(2).run();

        // 如果不是最后一列，点击右侧上方的通宝作为滑动成功的标志
        if (!is_last_col) {
            ret &= ProcessTask(*this, { "JieGarden@Roguelike@CoppersListSwipeFlagClick" }).run();
        }
        else {
            // 最后一列时先滑动到最右侧再识别
            swipe_copper_list_to_rightmost(2);
            // 点击中间的通宝避免干扰右侧识别
            ret &= ProcessTask(*this, { "JieGarden@Roguelike@CoppersListSwipeRightMostClick" }).run();
        }

        // 获取新图像并检查是否滑动成功
        image = ctrler()->get_image();
        if (image_last.data == image.data) {
            LogError << __FUNCTION__ << "| image not updated after swipe at column" << col;
            break;
        }
        image_last = image;

#ifdef ASST_DEBUG
        image_draw = image.clone();
#endif

        // 根据是否是最后一列选择识别中间列或右侧列
        RoguelikeCoppersAnalyzer::ColumnRole role = is_last_col ? RoguelikeCoppersAnalyzer::ColumnRole::Rightmost
                                                                : RoguelikeCoppersAnalyzer::ColumnRole::Middle;
        RoguelikeCoppersAnalyzer column_analyzer(image);
        // 分析当前列，检测已投出状态
        if (!column_analyzer.analyze_column(role, true)) {
            LogError << __FUNCTION__ << "| no coppers recognized in column" << col;
            continue;
        }

        const auto& detections = column_analyzer.get_detections();
        if (detections.empty()) {
            LogError << __FUNCTION__ << "| no coppers recognized in column" << col;
            continue;
        }

        // 获取列的度量信息用于坐标计算
        const auto& metrics = column_analyzer.get_column_metrics();
        update_column_coordinates(metrics, col, is_last_col);

        // 处理当前列的所有通宝
        for (size_t row = 0; row < detections.size(); ++row) {
            const auto& detection = detections[row];

            LogInfo << __FUNCTION__ << "| found copper:" << detection.name << "at (" << col << "," << row
                    << ") is_cast:" << detection.is_cast;

#ifdef ASST_DEBUG
            draw_detection_debug(image_draw, detection, cv::Scalar(0, 0, 255));
#endif

            // 从OCR结果创建通宝对象
            auto copper_opt = create_copper_from_name(
                detection.name,
                col,
                static_cast<int>(row + 1),
                detection.is_cast,
                detection.name_roi);
            if (!copper_opt) {
                LogError << __FUNCTION__ << "| failed to create copper at (" << col << "," << row << ")";
                continue;
            }

            auto copper = std::move(*copper_opt);
            copper.type = RoguelikeCoppersConfig::get_type_from_template(detection.templ_name);

            // 添加到现有通宝列表
            m_copper_list.emplace_back(std::move(copper));
        }

        // 如果不是最后一列，向右滑动一列继续扫描
        if (!is_last_col) {
            swipe_copper_list_right(1);
        }

#ifdef ASST_DEBUG
        save_debug_image(image_draw, "exchange");
#endif

        // 如果是最后一列，记录总列数并结束扫描
        if (is_last_col) {
            m_col = col;
            LogInfo << __FUNCTION__ << "| total columns detected:" << m_col;
            break;
        }
    }

    // 检查是否找到任何现有通宝
    if (m_copper_list.empty()) {
        LogError << __FUNCTION__ << "| no coppers found in list for comparison";
        // 直接进入next，放弃交换
        return CopperTaskResult::SKIPPED;
    }

    // =================================================
    // 第三步：使用策略决定是否交换并执行
    // =================================================

    // 使用策略决定是否交换
    size_t discard_index = 0;
    bool should_exchange = decide_exchange_by_strategy(m_new_copper, m_copper_list, discard_index);

    if (!should_exchange) {
        LogInfo << __FUNCTION__ << "| Decided not to exchange copper";
        return CopperTaskResult::SKIPPED;
    }

    // 确保索引有效
    if (discard_index >= m_copper_list.size()) {
        LogError << __FUNCTION__ << "| Invalid discard index:" << discard_index << ", total" << m_copper_list.size()
                 << "coppers";
        return CopperTaskResult::FAILED;
    }

    auto& worst_copper = m_copper_list[discard_index];

    auto strategy_name =
        (get_current_strategy_type() == CopperStrategyType::FindLingMode) ? "FindLing" : "Default";

    // 执行交换
    LogInfo << __FUNCTION__ << "| Exchanging copper using strategy: " << strategy_name << " - " << worst_copper.name
            << "(" << worst_copper.col << "," << worst_copper.row << ") -> " << m_new_copper.name;

    // 发送通宝替换信息到 WPF
    auto copper_info = basic_info_with_what("RoguelikeCoppersExchangeInfo");
    copper_info["details"]["to_discard"] =
        std::format("{} ({},{})", worst_copper.name, worst_copper.col, worst_copper.row);
    copper_info["details"]["to_pickup"] = m_new_copper.name;
    callback(AsstMsg::SubTaskExtraInfo, copper_info);

    // 点击要替换的通宝
    click_copper_at_position(worst_copper.col, worst_copper.row);

    // 执行确认交换任务
    ret &= ProcessTask(*this, { "JieGarden@Roguelike@CoppersTakeConfirm" }).run();

    return ret ? CopperTaskResult::SUCCESS : CopperTaskResult::FAILED;
}

// 滑动通宝列表指定次数
bool asst::RoguelikeCoppersTaskPlugin::swipe_copper_list(int times, bool to_left) const
{
    const int ERROR_THRESHOLD = 50; // 误差阈值，超过则进行校正滑动
    bool ret = true;
    for (int i = 0; i < times; ++i) {
        std::string task_name = to_left ? "JieGarden@Roguelike@CoppersListSlowlySwipeToTheLeft"
                                        : "JieGarden@Roguelike@CoppersListSlowlySwipeToTheRight";

        ret &= ProcessTask(*this, { task_name }).run();

        // 识别滑动效果，误差较大时额外滑动一次进行校准
        Matcher matcher(ctrler()->get_image());
        matcher.set_task_info("JieGarden@Roguelike@CoppersListSwipeErrorRecognition");
        if (matcher.analyze()) {
            int cur_x = matcher.get_result().rect.x;
            // m_origin_x 经典值: 572
            // cur_x 硬限制: [400,631]
            if (abs(m_origin_x - cur_x) >= ERROR_THRESHOLD) {
                Point origin_point = Point(m_origin_x, m_y);
                Point cur_point = Point(cur_x, m_y);
                auto swipe_task = Task.get("SlowlySwipeToTheRight");
                ret &= ctrler()->swipe(
                    cur_point,
                    origin_point,
                    swipe_task->special_params.empty() ? 0 : swipe_task->special_params.at(0),
                    (swipe_task->special_params.size() < 2) ? false : swipe_task->special_params.at(1),
                    (swipe_task->special_params.size() < 3) ? 1 : swipe_task->special_params.at(2),
                    (swipe_task->special_params.size() < 4) ? 1 : swipe_task->special_params.at(3));
                Log.debug(
                    __FUNCTION__,
                    std::format(
                        "| correcting swipe error: origin_x = {}, cur_x = {}, diff = {}",
                        m_origin_x,
                        cur_x,
                        abs(m_origin_x - cur_x)));
                ret &= sleep(100);
            }
        }
    }
    return ret;
}

// 向左滑动通宝列表指定次数
bool asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_left(int times) const
{
    return swipe_copper_list(times, true);
}

// 向右滑动通宝列表指定次数
bool asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_right(int times) const
{
    return swipe_copper_list(times, false);
}

bool asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_to_leftmost(int times) const
{
    bool ret = true;
    for (int i = 0; i < times; ++i) {
        std::string task_name = "JieGarden@Roguelike@CoppersListSwipeToTheLeft";
        ret &= ProcessTask(*this, { task_name }).run();
    }
    return ret;
}

bool asst::RoguelikeCoppersTaskPlugin::swipe_copper_list_to_rightmost(int times) const
{
    bool ret = true;
    for (int i = 0; i < times; ++i) {
        std::string task_name = "JieGarden@Roguelike@CoppersListSwipeToTheRight";
        ret &= ProcessTask(*this, { task_name }).run();
    }
    return ret;
}

// 根据行列位置计算并点击指定位置的通宝
void asst::RoguelikeCoppersTaskPlugin::click_copper_at_position(int col, int row) const
{
    // 根据列数选择X坐标：最后一列使用last_x，其他列使用m_x
    int x = col == m_col ? m_last_x : m_x;
    // 计算Y坐标：基于行偏移量
    Point click_point(x, m_y + (row - 1) * m_row_offset);

    Log.debug(
        __FUNCTION__,
        std::format(
            "| clicking copper at ({},{}) -> point ({},{},{},{})",
            col,
            row,
            click_point.x,
            m_y,
            (row - 1),
            m_row_offset));

    // 先滑动回最左边
    swipe_copper_list_to_leftmost(m_col + 1);
    sleep(300);

    // 再滑动到目标列
    swipe_copper_list_right(col - 1);

    // 执行点击
    ctrler()->click(click_point);
    sleep(300);
}

// 辅助函数：根据列类型更新坐标基准点
void asst::RoguelikeCoppersTaskPlugin::update_column_coordinates(
    const RoguelikeCoppersAnalyzer::ColumnMetrics& metrics,
    int col,
    bool is_last_col)
{
    // 根据列类型更新X坐标基准点：非最后一列使用m_x，最后一列记录last_x用于点击计算
    m_x = !is_last_col ? metrics.m_x : m_x;
    m_last_x = is_last_col ? metrics.m_x : m_last_x;

    // 更新统一的Y坐标基准点和行偏移量
    m_y = metrics.m_y;
    if (metrics.row_offset != 0) {
        m_row_offset = metrics.row_offset;
    }

    if (col == 1) {
        m_origin_x = m_x;
    }
}

// 根据识别到的名称创建通宝对象
std::optional<asst::RoguelikeCopper> asst::RoguelikeCoppersTaskPlugin::create_copper_from_name(
    const std::string& name,
    int col,
    int row,
    bool is_cast,
    const Rect& pos)
{
    // 从配置中查找通宝信息
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

    // 将识别到的错误的名称发送到 WPF 进行反馈
    auto copper_info = basic_info_with_what("RoguelikeCoppersRecognitionError");
    copper_info["details"]["recognized_name"] = name;
    callback(AsstMsg::SubTaskExtraInfo, copper_info);

    // 如果通宝不在配置中，保存调试图像
    try {
        cv::Mat screen = ctrler()->get_image();
        if (!screen.empty()) {
            cv::Mat screen_draw = screen.clone();
            cv::rectangle(screen_draw, cv::Rect(pos.x, pos.y, pos.width, pos.height), cv::Scalar(0, 0, 255), 2);

            save_debug_image(screen_draw, "unknown_draw", false);
        }
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, std::format("| failed to save unknown copper debug image: {}", e.what()));
    }

    return std::nullopt;
}

// 调试绘制辅助函数：在图像上绘制检测结果
void asst::RoguelikeCoppersTaskPlugin::draw_detection_debug(
    cv::Mat& image,
    const RoguelikeCoppersAnalyzer::CopperDetection& detection,
    const cv::Scalar& color) const
{
    // 绘制名称识别区域的矩形框
    cv::rectangle(
        image,
        cv::Rect(detection.name_roi.x, detection.name_roi.y, detection.name_roi.width, detection.name_roi.height),
        color,
        2);
    // 在矩形框上方显示名称识别置信度
    cv::putText(
        image,
        std::format("score: {:.6f}", detection.name_score),
        cv::Point(detection.name_roi.x, std::max(0, detection.name_roi.y - 6)),
        cv::FONT_HERSHEY_SIMPLEX,
        0.45,
        color,
        1);

    // 如果有已投出状态识别，也绘制相应的区域和置信度
    if (detection.is_cast) {
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

// 保存调试图像到文件
void asst::RoguelikeCoppersTaskPlugin::save_debug_image(
    const cv::Mat& image,
    const std::string& suffix,
    bool auto_clean) const
{
    try {
        const static std::vector<int> jpeg_params = { cv::IMWRITE_JPEG_QUALITY, 95, cv::IMWRITE_JPEG_OPTIMIZE, 1 };

        utils::save_debug_image(
            image,
            utils::path("debug") / "roguelike" / "coppers",
            auto_clean,
            "roguelikeCoppers debug",
            suffix,
            "jpeg",
            jpeg_params);
    }
    catch (const std::exception& e) {
        Log.error(__FUNCTION__, "| failed to save debug image:", e.what());
    }
}

// 获取当前游戏模式对应的交换策略
asst::CopperStrategyType asst::RoguelikeCoppersTaskPlugin::get_current_strategy_type() const
{
    // 检查当前游戏是否为FindLing模式
    // 检查是否是界园主题且为FindPlaytime（刷常乐节点）模式
    if (m_config->get_theme() == RoguelikeTheme::JieGarden && m_config->get_mode() == RoguelikeMode::FindPlaytime) {
        return CopperStrategyType::FindLingMode;
    }

    // 默认使用标准策略
    return CopperStrategyType::Default;
}

bool asst::RoguelikeCoppersTaskPlugin::decide_exchange_by_strategy(
    const RoguelikeCopper& new_copper,
    const std::vector<RoguelikeCopper>& existing_coppers,
    size_t& discard_index) const
{
    auto strategy = get_copper_strategy(get_current_strategy_type());
    auto result = strategy.exchange(new_copper, existing_coppers);

    LogInfo << __FUNCTION__ << result.reason;

    if (result.should_exchange) {
        discard_index = result.discard_index;
    }
    return result.should_exchange;
}
