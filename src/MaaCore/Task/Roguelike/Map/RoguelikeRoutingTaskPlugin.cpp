#include "RoguelikeRoutingTaskPlugin.h"

#include <limits>
#include <numeric>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/BrightPointAnalyzer.h"
#include "Vision/MultiMatcher.h"

bool asst::RoguelikeRoutingTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    LogTraceFunction;
    const std::string& theme = m_config->get_theme();

    // 本插件暂处于实验阶段，仅用于萨卡兹肉鸽
    if (theme != RoguelikeTheme::Sarkaz) {
        return false;
    }

    const std::shared_ptr<MatchTaskInfo> config = Task.get<MatchTaskInfo>(theme + "@Roguelike@RoutingConfig");

    m_origin_x = config->special_params.at(0);
    m_middle_x = config->special_params.at(1);
    m_last_x = config->special_params.at(2);
    m_node_width = config->special_params.at(3);
    m_node_height = config->special_params.at(4);
    m_column_offset = config->special_params.at(5);
    m_nameplate_offset = config->special_params.at(6);
    m_roi_margin = config->special_params.at(7);
    m_direction_threshold = config->special_params.at(8);

    return true;
}

void asst::RoguelikeRoutingTaskPlugin::reset_in_run_variables()
{
    m_map.reset();
    m_need_generate_map = true;
    m_selected_column = 0;
    m_selected_x = 0;
}

bool asst::RoguelikeRoutingTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task_name = details.get("details", "task", "");
    if (task_name == m_config->get_theme() + "@Roguelike@Routing") {
        return true;
    }

    return false;
}

bool asst::RoguelikeRoutingTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_need_generate_map) {
        generate_map();
        m_need_generate_map = false;
    }

    m_selected_column = m_map.get_node_column(m_map.get_curr_pos());
    update_selected_x();

    refresh_following_combat_nodes();
    navigate_route();

    return true;
}

void asst::RoguelikeRoutingTaskPlugin::generate_map()
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

    m_map.reset();
    size_t curr_col = m_map.init_index + 1;
    Rect roi = Task.get<MatchTaskInfo>(theme + "@Roguelike@RoutingNodeAnalyze")->roi;

    // 第一列节点
    cv::Mat image = ctrler()->get_image();
    MultiMatcher node_analyzer(image);
    node_analyzer.set_task_info(theme + "@Roguelike@RoutingNodeAnalyze");
    if (!node_analyzer.analyze()) {
        Log.error(__FUNCTION__, "| no nodes found in the first column");
        return;
    }
    MultiMatcher::ResultsVec match_results = node_analyzer.get_result();
    sort_by_vertical_(match_results); // 按照垂直方向排序（从上到下）
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
        sort_by_vertical_(match_results);
        for (const auto& [rect, score, templ_name] : match_results) {
            const RoguelikeNodeType type = RoguelikeMapInfo.templ2type(theme, templ_name);
            const size_t node = m_map.create_and_insert_node(type, curr_col, rect.y).value();
            generate_edges(node, image, rect.x);
        }
        ProcessTask(*this, { theme + "@Roguelike@RoutingSwipeRight" }).run();
        sleep(200);
        image = ctrler()->get_image();
        node_analyzer.set_image(image);
    }

    ProcessTask(*this, { theme + "@Roguelike@RoutingExitThenContinue" }).run(); // 通过退出重进回到初始位置

    return;
}

void asst::RoguelikeRoutingTaskPlugin::generate_edges(const size_t& node, const cv::Mat& image, const int& node_x)
{
    LogTraceFunction;

    const size_t node_column = m_map.get_node_column(node);

    if (node_column == m_map.init_index) {
        Log.error(__FUNCTION__, "| cannot generate edges for init node");
        return;
    }

    if (node_column == m_map.init_index + 1) {
        m_map.add_edge(m_map.init_index, node); // 第一列节点直接与 init 连接
        return;
    }

    // 将 image转换为二值图像后计算亮点
    BrightPointAnalyzer analyzer(image);

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
        analyzer.set_roi(roi);

        if (!analyzer.analyze()) { // 节点间没有连线
            continue;
        }

        // 按照水平方向排序（从左到右）
        std::vector<Point> brightPoints = analyzer.get_result();

        auto [x_min_p, x_max_p] = ranges::minmax(brightPoints, /*comp=*/ {}, [](const Point& p) { return p.x; });
        const int leftmost_x = x_min_p.x;
        const int rightmost_x = x_max_p.x;

        auto leftmostBrightPoints = brightPoints | views::filter([&](const Point& p) { return p.x == leftmost_x; });
        auto rightmostBrightPoints = brightPoints | views::filter([&](const Point& p) { return p.x == rightmost_x; });

        auto [leftmost_y_min_p, leftmost_y_max_p] =
            ranges::minmax(leftmostBrightPoints, /*comp=*/ {}, [](const Point& p) { return p.y; });
        const int leftmost_y = (leftmost_y_min_p.y + leftmost_y_max_p.y) / 2;

        auto [rightmost_y_min_p, rightmost_y_max_p] =
            ranges::minmax(rightmostBrightPoints, /*comp=*/ {}, [](const Point& p) { return p.y; });
        const int rightmost_y = (rightmost_y_min_p.y + rightmost_y_max_p.y) / 2;

        if (std::abs(prev_y - node_y) < m_direction_threshold &&
            std::abs(leftmost_y - rightmost_y) < m_direction_threshold) {
            m_map.add_edge(prev, node);
        }
        else if (prev_y < node_y && leftmost_y < rightmost_y - m_direction_threshold) {
            m_map.add_edge(prev, node);
        }
        else if (prev_y > node_y && leftmost_y > rightmost_y + m_direction_threshold) {
            m_map.add_edge(prev, node);
        }
    }

    // 同列前一个节点
    if (node > m_map.get_column_begin(node_column)) {
        size_t prev = node - 1;
        roi.x = node_x + m_node_width / 2 - m_roi_margin;
        roi.y = (m_map.get_node_y(prev) + m_node_height + m_nameplate_offset + node_y) / 2 - m_roi_margin;
        analyzer.set_roi(roi);
        if (analyzer.analyze()) {
            m_map.add_edge(prev, node);
            m_map.add_edge(node, prev);
        }
    }
}

void asst::RoguelikeRoutingTaskPlugin::refresh_following_combat_nodes()
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
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@RoutingRefreshNode" }).run();
        m_map.set_node_refresh_times(next_node, m_map.get_node_refresh_times(next_node) + 1);

        // 识别并更新节点类型
        Matcher node_analyzer(ctrler()->get_image());
        node_analyzer.set_task_info(theme + "@Roguelike@RoutingNodeAnalyze");
        node_analyzer.set_roi(next_node_rect);
        if (node_analyzer.analyze()) {
            Matcher::Result match_results = node_analyzer.get_result();
            m_map.set_node_type(next_node, RoguelikeMapInfo.templ2type(theme, match_results.templ_name));
        }
    }
}

void asst::RoguelikeRoutingTaskPlugin::navigate_route()
{
    LogTraceFunction;

    const size_t curr_column = m_map.get_node_column(m_map.get_curr_pos());

    m_map.set_cost_fun([&](const RoguelikeNodePtr& node) {
        if (node->visited) {
            return 1000;
        }

        if (node->column == curr_column) {
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
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@ExitThenAbandon");
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
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@RoutingActionEncounter");
        m_map.set_curr_pos(next_node);
    }
    else if (m_map.get_node_type(next_node) == RoguelikeNodeType::RogueTrader) {
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@RoutingActionRogueTrader");
        reset_in_run_variables();
    }
    else {
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@ExitThenAbandon");
        reset_in_run_variables();
    }
}

void asst::RoguelikeRoutingTaskPlugin::update_selected_x()
{
    if (m_selected_column == m_map.init_index) {
        m_selected_x = m_origin_x - m_column_offset;
    }
    else if (m_selected_column == m_map.init_index + 1) {
        m_selected_x = m_origin_x;
    }
    else if (m_selected_column == m_map.get_num_columns() - 1) [[unlikely]] {
        m_selected_x = m_last_x;
    }
    else {
        m_selected_x = m_middle_x;
    }
}
