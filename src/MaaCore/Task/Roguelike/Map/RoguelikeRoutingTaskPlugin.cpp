#include "RoguelikeRoutingTaskPlugin.h"

#include <limits>
#include <numeric>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/PixelAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"

bool asst::RoguelikeRoutingTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    const std::string& theme = m_config->get_theme();

    // 本插件暂处于实验阶段，仅用于萨卡兹和界园肉鸽的第一层
    if (theme != RoguelikeTheme::Sarkaz && theme != RoguelikeTheme::JieGarden) {
        return false;
    }

    const TaskPtr config_task = Task.get("RoguelikeRoutingConfig");

    m_origin_x = config_task->special_params.at(0);
    m_middle_x = config_task->special_params.at(1);
    m_last_x = config_task->special_params.at(2);
    m_node_width = config_task->special_params.at(3);
    m_node_height = config_task->special_params.at(4);
    m_column_offset = config_task->special_params.at(5);
    m_nameplate_offset = config_task->special_params.at(6);
    m_roi_margin = config_task->special_params.at(7);
    m_direction_threshold = config_task->special_params.at(8);

    const RoguelikeMode& mode = m_config->get_mode();
    const std::string squad = params.get("squad", "");

    if (theme == RoguelikeTheme::Sarkaz && mode == RoguelikeMode::FastPass && squad == "蓝图测绘分队") {
        m_routing_strategy = RoutingStrategy::Sarkaz_FastPass;
        return true;
    }

    if (theme == RoguelikeTheme::Sarkaz && mode == RoguelikeMode::Investment && squad == "点刺成锭分队") {
        m_routing_strategy = RoutingStrategy::Sarkaz_FastInvestment;
        return true;
    }

    if (theme == RoguelikeTheme::JieGarden) {
        // 商贾分队 + 刷源石锭：重投通宝刷出诡意行商存款（两条件缺一不可，由游戏机制决定）
        if (mode == RoguelikeMode::Investment && squad == "商贾分队") {
            m_routing_strategy = RoutingStrategy::JieGarden_MerchantRecast;
            return true;
        }
        if (((mode == RoguelikeMode::Investment && squad == "指挥分队") ||
             (mode == RoguelikeMode::Collectible && params.get("collectible_mode_squad", squad) == "指挥分队")) &&
            m_config->get_difficulty() >= 3) {
            m_routing_strategy = RoutingStrategy::JieGarden_FastPassWithBattle;
            return true;
        }
    }

    return false;
}

void asst::RoguelikeRoutingTaskPlugin::reset_in_run_variables()
{
    m_map.reset();
    m_need_generate_map = true;
    m_selected_column = 0;
    m_selected_x = 0;
    m_token_cast_at_start = false;
    m_start_auto_cast_checked = false;
}

bool asst::RoguelikeRoutingTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    std::string task_name = details.get("details", "task", "");

    // trigger 任务的名字可以为 "...@Roguelike@Routing-..." 的形式
    if (const size_t pos = task_name.find('-'); pos != std::string::npos) {
        task_name = task_name.substr(0, pos);
    }

    if (task_name == m_config->get_theme() + "@Roguelike@Routing") {
        m_verify_start_auto_cast = false;
        return true;
    }

    // 商贾分队：进图后游戏自动投钱，结果弹窗由 RandomPickAfterNextLevel 点确定关闭。
    // 在它点掉之前拦一次（每局仅一次），OCR 判断开局是否已投出「触锁代币」。
    if (m_routing_strategy == RoutingStrategy::JieGarden_MerchantRecast && !m_start_auto_cast_checked &&
        task_name == m_config->get_theme() + "@Roguelike@RandomPickAfterNextLevel") {
        m_verify_start_auto_cast = true;
        return true;
    }

    return false;
}

bool asst::RoguelikeRoutingTaskPlugin::_run()
{
    LogTraceFunction;

    // 商贾分队：本次由开局自投结果弹窗触发，先 OCR 判断再放行其点确定（见 detect_start_auto_cast）
    if (m_verify_start_auto_cast) {
        detect_start_auto_cast();
        return true;
    }

    switch (m_routing_strategy) {
    case RoutingStrategy::Sarkaz_FastInvestment:
        if (m_need_generate_map) {
            // 随机点击一个第一列的节点，先随便写写，垃圾代码迟早要重构
            ProcessTask(*this, { "Sarkaz@RoguelikeRouting-CombatOps" }).run();
            // 刷新节点
            ProcessTask(*this, { "Sarkaz@RoguelikeRouting-RefreshNode" }).run();
            // 不识别了，进商店，Go!
            Task.set_task_base("RoguelikeRoutingAction", "Sarkaz@RoguelikeRoutingAction-StageTraderEnter");
            // 偷懒，直接用 m_need_generate_map 判断是否已进过商店
            m_need_generate_map = false;
        }
        else {
            Task.set_task_base("RoguelikeRoutingAction", "Sarkaz@RoguelikeRoutingAction-ExitThenAbandon");
        }
        break;
    case RoutingStrategy::JieGarden_FastPassWithBattle:
        if (m_need_generate_map) {
            // 向左滑动以检视前三列节点
            ProcessTask(*this, { "RoguelikeRouting-MoveRightToPeek" }).run();
            cv::Mat image = ctrler()->get_image();
            cv::Mat image_draw = image.clone();
            update_map(image, RoguelikeMap::INIT_INDEX + 1, image_draw);
#ifdef ASST_DEBUG
            utils::save_debug_image(
                image_draw,
                utils::path("debug") / "roguelikeMap",
                /*auto_clean=*/true,
                /*description=*/"bosky map draw",
                /*suffix=*/"draw");
#endif
            m_need_generate_map = false;

            // 根据第三列节点类型更新导航策略
            const size_t sample_node_of_last_column = m_map.size() - 1;
            const RoguelikeNodeType sample_node_type = m_map.get_node_type(sample_node_of_last_column);
            Log.info("RoguelikeRouting | Type of last node:", type2name(sample_node_type));
            if (sample_node_type == RoguelikeNodeType::RogueTrader) {
                m_routing_strategy = RoutingStrategy::JieGarden_FastPassWithoutBattle;
                m_config->set_skip_recruit_in_fast_pass(true);
                return _run();
            }
        }
        if (m_map.get_curr_pos() == RoguelikeMap::INIT_INDEX) {
            // 规划路线
            m_map.set_cost_fun([&](const RoguelikeNodePtr& node) {
                if (node->type == RoguelikeNodeType::CombatOps) {
                    return 10;
                }
                if (node->type == RoguelikeNodeType::EmergencyOps || node->type == RoguelikeNodeType::DreadfulFoe) {
                    return 11;
                }
                return 0;
            });
            m_map.update_node_costs();
            const size_t next_node = m_map.get_next_node();

            // 若无法避免超过三场战斗则重开
            if (m_map.get_node_cost(next_node) >= 30) {
                callback(
                    AsstMsg::TaskChainExtraInfo,
                    json::object {
                        { "what", "RoutingRestart" },
                        { "why", "TooManyBattlesAhead" },
                        { "node_cost", m_map.get_node_cost(next_node) },
                    });

                Task.set_task_base("RoguelikeRoutingAction", "JieGarden@RoguelikeRoutingAction-ExitThenAbandon");
            }
            else {
                const int next_node_x = m_left_most_column_x_in_view;
                const int next_node_y = m_map.get_node_y(next_node);
                Point next_node_center = Point(next_node_x + m_node_width / 2, next_node_y + m_node_height / 2);
                ctrler()->click(next_node_center);
                sleep(200);

                Task.set_task_base(
                    "RoguelikeRoutingAction",
                    "JieGarden@RoguelikeRoutingAction-StageCombatOpsEnterThenLeave");
                m_map.set_curr_pos(next_node);
            }
        }
        else {
            // 执行默认的避战策略
            Task.set_task_base("RoguelikeRoutingAction", "JieGarden@Roguelike@Stages_default");
        }
        break;

    case RoutingStrategy::JieGarden_FastPassWithoutBattle:
        if (m_need_generate_map) {
            cv::Mat image = ctrler()->get_image();
            cv::Mat image_draw = image.clone();
            update_map(image, RoguelikeMap::INIT_INDEX + 1, image_draw);
#ifdef ASST_DEBUG
            utils::save_debug_image(
                image_draw,
                utils::path("debug") / "roguelikeMap",
                /*auto_clean=*/true,
                /*description=*/"bosky map draw",
                /*suffix=*/"draw");
#endif
            m_need_generate_map = false;
        }
        if (m_map.get_curr_pos() == RoguelikeMap::INIT_INDEX) {
            m_map.set_cost_fun([&](const RoguelikeNodePtr& node) {
                if (node->type == RoguelikeNodeType::CombatOps || node->type == RoguelikeNodeType::EmergencyOps ||
                    node->type == RoguelikeNodeType::DreadfulFoe) {
                    return 1;
                }
                return 0;
            });
            m_map.update_node_costs();
            const size_t next_node = m_map.get_next_node();

            // 若无法避免超过两场战斗则重开
            if (m_map.get_node_cost(next_node) >= 2) {
                callback(
                    AsstMsg::TaskChainExtraInfo,
                    json::object {
                        { "what", "RoutingRestart" },
                        { "why", "TooManyBattlesAhead" },
                        { "node_cost", m_map.get_node_cost(next_node) },
                    });

                Task.set_task_base("RoguelikeRoutingAction", "JieGarden@RoguelikeRoutingAction-ExitThenAbandon");
            }
            else {
                const int next_node_x = m_left_most_column_x_in_view;
                const int next_node_y = m_map.get_node_y(next_node);
                Point next_node_center = Point(next_node_x + m_node_width / 2, next_node_y + m_node_height / 2);
                ctrler()->click(next_node_center);
                sleep(200);

                Task.set_task_base(
                    "RoguelikeRoutingAction",
                    "JieGarden@RoguelikeRoutingAction-StageCombatOpsEnterThenLeave");
                m_map.set_curr_pos(next_node);
            }
        }
        else {
            // 执行默认的避战策略
            Task.set_task_base("RoguelikeRoutingAction", "JieGarden@Roguelike@Stages_default");
        }
        break;
    case RoutingStrategy::Sarkaz_FastPass:
        if (m_need_generate_map) {
            generate_map();
            m_need_generate_map = false;
        }

        m_selected_column = m_map.get_node_column(m_map.get_curr_pos());
        update_selected_x();

        refresh_following_combat_nodes();
        navigate_route();
        break;

    case RoutingStrategy::JieGarden_MerchantRecast:
        if (m_need_generate_map) {
            // 1. 循环重投通宝，直到“衡-触锁代币”已投出（票券耗尽则回退）
            if (recast_until_token_cast()) {
                // 开局自投路径跳过了开盒，地图未被复位到初始位置（重投路径靠开/关钱盒复位到最左）。
                // 先手动把地图滑到最左，使下面 MoveRightToPeek 的起点与重投路径一致，否则 update_map
                // 测到的第一列横坐标整体左偏（实测 321→121），update_selected_x 仍按 m_origin_x
                // 选点会点空、刷新键点不到。
                if (m_token_cast_at_start) {
                    ProcessTask(*this, { "RoguelikeRouting-MoveToLeftmost" }).run();
                }
                // 2. 解析地图并选中战斗节点后刷新：refresh_following_combat_nodes 会先 click 选中节点、
                //    再点刷新键并确认；触锁代币使被刷新节点变为诡意行商，且刷新后该节点仍处于选中态
                ProcessTask(*this, { "RoguelikeRouting-MoveRightToPeek" }).run();
                {
                    const cv::Mat map_image = ctrler()->get_image();
                    update_map(map_image, RoguelikeMap::INIT_INDEX + 1);
                }
                m_selected_column = m_map.get_node_column(m_map.get_curr_pos());
                update_selected_x();
                refresh_following_combat_nodes(/*only_first=*/true);
                // 3. 节点仍选中，直接进入诡意行商（不再寻路另选节点）；存款由 InvestPlugin 自动完成，之后走 else 退出
                Task.set_task_base("RoguelikeRoutingAction", "JieGarden@RoguelikeRoutingAction-StageTraderEnter");
            }
            else {
                // 票券耗尽仍未投出：放弃本轮、退出重开下一轮
                // （若想“正常打完本轮”，可在此改为回退通用投资/避战策略）
                Task.set_task_base("RoguelikeRoutingAction", "JieGarden@RoguelikeRoutingAction-ExitThenAbandon");
            }
            m_need_generate_map = false;
        }
        else {
            // 存款完成 -> 退出放弃，进入下一轮
            Task.set_task_base("RoguelikeRoutingAction", "JieGarden@RoguelikeRoutingAction-ExitThenAbandon");
        }
        break;

    default:
        break;
    }

    return true;
}

bool asst::RoguelikeRoutingTaskPlugin::update_map(
    const cv::Mat& image,
    const size_t leftmost_column,
    std::optional<std::reference_wrapper<cv::Mat>> image_draw_opt)
{
    LogTraceFunction;

    if (leftmost_column == 0) {
        Log.error(__FUNCTION__, "| leftmost_column must be greater than zero");
        return false;
    }

    const std::string& theme = m_config->get_theme();

    size_t curr_col = leftmost_column - 1;
    int curr_x = -m_node_width - 1; // 第一列节点将触发 rect.x >= curr_x + m_node_width 并更新 curr_col 与 curr_x

    MultiMatcher node_analyzer(image);
    node_analyzer.set_task_info(theme + "@RoguelikeRoutingNodeAnalyze");
    if (!node_analyzer.analyze()) {
        Log.error(__FUNCTION__, "| no nodes are recognised");
        return false;
    }
    MultiMatcher::ResultsVec match_results = node_analyzer.get_result();
    sort_by_vertical_(match_results); // 按照水平方向从左到右排序各列节点，同一列节点按照垂直方向从上到下排序
    m_left_most_column_x_in_view = match_results.front().rect.x;

    const size_t old_num_columns = m_map.get_num_columns();
    for (const auto& [rect, score, templ_name] : match_results) {
        const RoguelikeNodeType type = RoguelikeMapInfo.templ2type(theme, templ_name);
#ifdef ASST_DEBUG
        if (image_draw_opt.has_value()) {
            cv::rectangle(image_draw_opt.value().get(), make_rect<cv::Rect>(rect), cv::Scalar(255, 255, 255), 2);
            cv::putText(
                image_draw_opt.value().get(),
                templ_name,
                cv::Point(rect.x, rect.y - m_roi_margin),
                cv::FONT_HERSHEY_DUPLEX,
                0.5,
                cv::Scalar(255, 255, 255));
        }
#endif
        if (rect.x >= curr_x + m_node_width) { // 识别到下一列的节点
            ++curr_col;
            curr_x = rect.x;
        }
        if (curr_col >= old_num_columns) // 仅更新新列节点
        {
            const size_t node = m_map.create_and_insert_node(type, curr_col, rect.y).value();
            generate_edges(node, image, rect.x, image_draw_opt);
        }
    }

    return true;
}

void asst::RoguelikeRoutingTaskPlugin::generate_map()
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

    m_map.reset();
    size_t curr_col = RoguelikeMap::INIT_INDEX + 1;
    Rect roi = Task.get<MatchTaskInfo>(theme + "@RoguelikeRoutingNodeAnalyze")->roi;

    // 第一列节点
    cv::Mat image = ctrler()->get_image();
    MultiMatcher node_analyzer(image);
    node_analyzer.set_task_info(theme + "@RoguelikeRoutingNodeAnalyze");
    if (!node_analyzer.analyze()) {
        Log.error(__FUNCTION__, "| no nodes found in the first column");
        return;
    }
    MultiMatcher::ResultsVec match_results = node_analyzer.get_result();
    sort_by_horizontal_(match_results); // 按照垂直方向排序（从上到下）
    for (const auto& [rect, score, templ_name] : match_results) {
        const RoguelikeNodeType type = RoguelikeMapInfo.templ2type(theme, templ_name);
        const size_t node = m_map.create_and_insert_node(type, curr_col, rect.y).value();
        generate_edges(node, image, rect.x);
    }

    // 第二列及以后的节点
    roi.x += m_column_offset;
    node_analyzer.set_roi(roi);
    while (!need_exit() && node_analyzer.analyze()) {
        ++curr_col;
        match_results = node_analyzer.get_result();
        sort_by_horizontal_(match_results);
        for (const auto& [rect, score, templ_name] : match_results) {
            const RoguelikeNodeType type = RoguelikeMapInfo.templ2type(theme, templ_name);
            const size_t node = m_map.create_and_insert_node(type, curr_col, rect.y).value();
            generate_edges(node, image, rect.x);
        }
        ProcessTask(*this, { "RoguelikeRouting-MoveRight" }).run();
        sleep(200);
        image = ctrler()->get_image();
        node_analyzer.set_image(image);
    }

    ProcessTask(*this, { theme + "@RoguelikeRouting-ExitThenContinue" }).run(); // 通过退出重进回到初始位置
}

void asst::RoguelikeRoutingTaskPlugin::generate_edges(
    const size_t& node,
    const cv::Mat& image,
    const int& node_x,
    [[maybe_unused]] std::optional<std::reference_wrapper<cv::Mat>> image_draw_opt)
{
    LogTraceFunction;

    const size_t node_column = m_map.get_node_column(node);

    if (node_column == RoguelikeMap::INIT_INDEX) {
        Log.error(__FUNCTION__, "| cannot generate edges for init node");
        return;
    }

    if (node_column == RoguelikeMap::INIT_INDEX + 1) {
        m_map.add_edge(RoguelikeMap::INIT_INDEX, node); // 第一列节点直接与 init 连接
        return;
    }

    // 将 image转换为二值图像后计算亮点
    PixelAnalyzer analyzer(image);

    const int center_x = node_x - (m_column_offset - m_node_width) / 2; // node 与 前一列节点的中点横坐标
    const int node_y = m_map.get_node_y(node);
    Rect roi(0, 0, m_roi_margin * 2, m_roi_margin * 2);

    // 遍历前一列节点
    const size_t pre_col_begin = m_map.get_column_begin(node_column - 1);
    const size_t pre_col_end = m_map.get_column_end(node_column - 1);
    for (size_t prev = pre_col_begin; prev < pre_col_end; ++prev) {
        const int prev_y = m_map.get_node_y(prev);
        const int center_y = (prev_y + node_y + m_node_height) / 2;
        roi.x = center_x - m_roi_margin;
        roi.y = center_y - m_roi_margin;
#ifdef ASST_DEBUG
        if (image_draw_opt.has_value()) {
            cv::rectangle(image_draw_opt.value().get(), make_rect<cv::Rect>(roi), cv::Scalar(255, 255, 255), 1);
        }
#endif
        analyzer.set_roi(roi);

        if (!analyzer.analyze()) { // 节点间没有连线
            continue;
        }

        // 按照水平方向排序（从左到右）
        std::vector<Point> brightPixels = analyzer.get_result();

        auto [x_min_p, x_max_p] = std::ranges::minmax(brightPixels, /*comp=*/ {}, [](const Point& p) { return p.x; });
        const int leftmost_x = x_min_p.x;
        const int rightmost_x = x_max_p.x;

        auto leftmostBrightPixels =
            brightPixels | std::views::filter([&](const Point& p) { return p.x == leftmost_x; });
        auto rightmostBrightPixels =
            brightPixels | std::views::filter([&](const Point& p) { return p.x == rightmost_x; });

        auto [leftmost_y_min_p, leftmost_y_max_p] =
            std::ranges::minmax(leftmostBrightPixels, /*comp=*/ {}, [](const Point& p) { return p.y; });
        const int leftmost_y = (leftmost_y_min_p.y + leftmost_y_max_p.y) / 2;

        auto [rightmost_y_min_p, rightmost_y_max_p] =
            std::ranges::minmax(rightmostBrightPixels, /*comp=*/ {}, [](const Point& p) { return p.y; });
        const int rightmost_y = (rightmost_y_min_p.y + rightmost_y_max_p.y) / 2;

        if ((std::abs(prev_y - node_y) < m_direction_threshold &&
             std::abs(leftmost_y - rightmost_y) < m_direction_threshold) ||
            (prev_y < node_y && leftmost_y < rightmost_y - m_direction_threshold) ||
            (prev_y > node_y && leftmost_y > rightmost_y + m_direction_threshold)) {
            m_map.add_edge(prev, node);
#ifdef ASST_DEBUG
            if (image_draw_opt.has_value()) {
                cv::line(
                    image_draw_opt.value().get(),
                    cv::Point(node_x - m_column_offset + m_node_width / 2, prev_y + m_node_height / 2),
                    cv::Point(node_x + m_node_width / 2, node_y + m_node_height / 2),
                    cv::Scalar(255, 255, 255),
                    2);
            }
#endif
        }
    }

    // 同列前一个节点
    if (node > m_map.get_column_begin(node_column)) {
        size_t prev = node - 1;
        roi.x = node_x + m_node_width / 2 - m_roi_margin;
        roi.y = (m_map.get_node_y(prev) + m_node_height + m_nameplate_offset + node_y) / 2 - m_roi_margin;
#ifdef ASST_DEBUG
        if (image_draw_opt.has_value()) {
            cv::rectangle(image_draw_opt.value().get(), make_rect<cv::Rect>(roi), cv::Scalar(255, 255, 255), 1);
        }
#endif
        analyzer.set_roi(roi);
        if (analyzer.analyze()) {
            m_map.add_edge(prev, node);
            m_map.add_edge(node, prev);
#ifdef ASST_DEBUG
            if (image_draw_opt.has_value()) {
                cv::line(
                    image_draw_opt.value().get(),
                    cv::Point(node_x + m_node_width / 2, m_map.get_node_y(prev) + m_node_height / 2),
                    cv::Point(node_x + m_node_width / 2, node_y + m_node_height / 2),
                    cv::Scalar(255, 255, 255),
                    2);
            }
#endif
        }
    }
}

void asst::RoguelikeRoutingTaskPlugin::refresh_following_combat_nodes(bool only_first)
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

    const size_t curr_node = m_map.get_curr_pos();
    const size_t curr_node_column = m_map.get_node_column(curr_node);

    for (size_t next_node : m_map.get_node_succs(curr_node)) {
        // 不刷新同一列的节点
        const size_t next_node_column = m_map.get_node_column(next_node);
        if (next_node_column <= curr_node_column) {
            continue;
        }
        // 每个节点仅刷新一次
        if (m_map.get_node_refresh_times(next_node)) {
            continue;
        }
        // 不刷新非战斗节点
        RoguelikeNodeType next_node_type = m_map.get_node_type(next_node);
        if (next_node_type != RoguelikeNodeType::CombatOps && next_node_type != RoguelikeNodeType::EmergencyOps &&
            next_node_type != RoguelikeNodeType::DreadfulFoe) {
            continue;
        }

        int next_node_x = m_selected_x + (next_node_column == m_selected_column ? 0 : m_column_offset);
        int next_node_y = m_map.get_node_y(next_node);
        Rect next_node_rect = Rect(next_node_x, next_node_y, m_node_width, m_node_height);

        // 点击节点
        ctrler()->click(next_node_rect);
        m_selected_column = m_map.get_node_column(next_node);
        update_selected_x();
        next_node_rect.x = m_selected_x;
        sleep(200);

        // 刷新节点
        ProcessTask(*this, { m_config->get_theme() + "@RoguelikeRouting-RefreshNode" }).run();
        m_map.set_node_refresh_times(next_node, m_map.get_node_refresh_times(next_node) + 1);

        // 识别并更新节点类型
        Matcher node_analyzer(ctrler()->get_image());
        node_analyzer.set_task_info(theme + "@RoguelikeRoutingNodeAnalyze");
        node_analyzer.set_roi(next_node_rect);
        if (node_analyzer.analyze()) {
            const Matcher::Result& match_results = node_analyzer.get_result();
            m_map.set_node_type(next_node, RoguelikeMapInfo.templ2type(theme, match_results.templ_name));
        }

        // 商贾分队：触锁代币只够刷 1 次，刷完第一个战斗节点即停，保持该节点选中以便直接进店
        if (only_first) {
            break;
        }
    }
}

void asst::RoguelikeRoutingTaskPlugin::detect_start_auto_cast()
{
    LogTraceFunction;
    // 每局只检测一次（RandomPickAfterNextLevel 会点 #self 多次匹配，避免重复 OCR/等待）
    m_start_auto_cast_checked = true;

    // 自投结果弹窗与重投结果页同一 UI（对号同在 [1033,462] 处）。
    // 等通宝名字渲染完再 OCR，否则会扫到空白漏判（复用重投结果页的等待时长）。
    sleep(Task.get("JieGarden@Roguelike@CoppersRecastResultAppear")->post_delay);

    OCRer token_ocr(ctrler()->get_image());
    token_ocr.set_task_info("JieGarden@Roguelike@CoppersRecastResultTokenOCR");
    if (token_ocr.analyze().has_value()) {
        m_token_cast_at_start = true;
        LogInfo << __FUNCTION__ << "| 开局自动投已投出「触锁代币」，将跳过开盒重投";
    }
    else {
        LogInfo << __FUNCTION__ << "| 开局自动投未投出「触锁代币」，进图后照常开盒重投";
    }
}

bool asst::RoguelikeRoutingTaskPlugin::recast_until_token_cast()
{
    LogTraceFunction;

    // 开局自动投已投出「触锁代币」：无需开盒重投，直接沿用（省票券、避免重投覆盖掉它）
    if (m_token_cast_at_start) {
        LogInfo << __FUNCTION__ << "| 开局已投出「触锁代币」，跳过开盒重投";
        return true;
    }

    // 票券很少，设安全上限兜底，避免死循环
    constexpr int max_recast_times = 5;

    // 打开钱盒
    if (!ProcessTask(*this, { "JieGarden@Roguelike@CoppersOpenBox" }).run()) {
        LogWarn << __FUNCTION__ << "| 无法打开钱盒，放弃重投";
        return false;
    }

    bool cast = false;
    for (int i = 0; i < max_recast_times; ++i) {
        // 重投并停在投钱结果页（不点对号）；票券耗尽/重投键不可用时 run() 返回 false
        if (!ProcessTask(*this, { "JieGarden@Roguelike@CoppersRecastToResult" }).run()) {
            LogWarn << __FUNCTION__ << "| 重投不可用（票券耗尽?），停止重投";
            break;
        }
        // 一次性 OCR 投钱结果页是否投出了「触锁代币」（结果页含三个名字，任一命中即可）
        OCRer token_ocr(ctrler()->get_image());
        token_ocr.set_task_info("JieGarden@Roguelike@CoppersRecastResultTokenOCR");
        const bool found = token_ocr.analyze().has_value();
        // 点对号确认/关闭结果页（不接掉落交换尾链）
        ProcessTask(*this, { "JieGarden@Roguelike@CoppersRecastResultConfirm" }).run();
        if (found) {
            LogInfo << __FUNCTION__ << "| 第 " << (i + 1) << " 次重投已投出「触锁代币」";
            cast = true;
            break;
        }
    }

    // 无论是否投出，都退出钱盒回到地图（刷新节点/进商店等后续操作需在地图界面进行）
    ProcessTask(*this, { "JieGarden@Roguelike@CoppersCloseBox" }).run();
    return cast;
}

void asst::RoguelikeRoutingTaskPlugin::navigate_route()
{
    LogTraceFunction;

    const size_t curr_col = m_map.get_node_column(m_map.get_curr_pos());

    m_map.set_cost_fun([&](const RoguelikeNodePtr& node) {
        if (node->visited) {
            return 1000;
        }

        if (node->column == curr_col) {
            return 1000;
        }

        if (node->type == RoguelikeNodeType::CombatOps || node->type == RoguelikeNodeType::EmergencyOps ||
            node->type == RoguelikeNodeType::DreadfulFoe) {
            return 1 + (node->refresh_times ? 999 : 0);
        }

        return 0;
    });

    m_map.update_node_costs();

    const size_t next_node = m_map.get_next_node();

    if (m_map.get_node_cost(next_node) >= 1000) {
        Task.set_task_base("RoguelikeRoutingAction", "Sarkaz@RoguelikeRoutingAction-ExitThenAbandon");
        reset_in_run_variables();
        return;
    }

    const size_t next_node_column = m_map.get_node_column(next_node);
    const int next_node_x = m_selected_x + (next_node_column == m_selected_column ? 0 : m_column_offset);
    const int next_node_y = m_map.get_node_y(next_node);
    Point next_node_center = Point(next_node_x + m_node_width / 2, next_node_y + m_node_height / 2);
    ctrler()->click(next_node_center);
    sleep(200);

    if (m_map.get_node_type(next_node) == RoguelikeNodeType::Encounter) {
        Task.set_task_base("RoguelikeRoutingAction", "Sarkaz@RoguelikeRoutingAction-StageEncounterEnter");
        m_map.set_curr_pos(next_node);
    }
    else if (m_map.get_node_type(next_node) == RoguelikeNodeType::RogueTrader) {
        Task.set_task_base("RoguelikeRoutingAction", "Sarkaz@RoguelikeRoutingAction-StageTraderEnter");
        reset_in_run_variables();
    }
    else {
        Task.set_task_base("RoguelikeRoutingAction", "Sarkaz@RoguelikeRoutingAction-ExitThenAbandon");
        reset_in_run_variables();
    }
}

void asst::RoguelikeRoutingTaskPlugin::update_selected_x()
{
    if (m_selected_column == RoguelikeMap::INIT_INDEX) {
        m_selected_x = m_origin_x - m_column_offset;
    }
    else if (m_selected_column == RoguelikeMap::INIT_INDEX + 1) {
        m_selected_x = m_origin_x;
    }
    else if (m_selected_column == m_map.get_num_columns() - 1) [[unlikely]] {
        m_selected_x = m_last_x;
    }
    else {
        m_selected_x = m_middle_x;
    }
}
