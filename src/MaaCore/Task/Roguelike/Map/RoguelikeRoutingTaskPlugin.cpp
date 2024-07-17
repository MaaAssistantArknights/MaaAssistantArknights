#include "RoguelikeRoutingTaskPlugin.h"
#include "Controller/Controller.h"
#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
#include "Vision/Matcher.h"
#include <limits>

#include "Utils/Logger.hpp"

asst::RoguelikeRoutingTaskPlugin::RoguelikeRoutingTaskPlugin(const AsstCallback& callback,
                                                             Assistant* inst,
                                                             std::string_view task_chain,
                                                             std::shared_ptr<RoguelikeConfig> config)
        : AbstractRoguelikeTaskPlugin(callback, inst, task_chain, config) {
    const std::string& theme = config->get_theme();

    std::shared_ptr<OcrTaskInfo> config_task =
        Task.get<OcrTaskInfo>(theme + "@Roguelike@MapNodeAnalyzeConfig");
    for (const auto& [templ_name, type_value] : config_task->replace_map) {
        m_templ_type_map.insert_or_assign(templ_name, static_cast<RoguelikeNodeType>(std::stoi(type_value)));
    }
    m_swipe_source.x = config_task->roi.x;
    m_swipe_source.y = config_task->roi.y;
    m_swipe_target.x = config_task->roi.width;
    m_swipe_target.y = config_task->roi.height;
    m_origin_x = config_task->special_params.at(0);
    m_node_width = config_task->special_params.at(1);
    m_node_height = config_task->special_params.at(2);
    m_column_offset = config_task->special_params.at(3);
    m_nameplate_offset = config_task->special_params.at(4);
    m_roi_margin = config_task->special_params.at(5);
    m_direction_threshold = config_task->special_params.at(6);
}

bool asst::RoguelikeRoutingTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }

    if (m_config->get_theme() != RoguelikeTheme::Sarkaz || m_config->get_mode() != RoguelikeMode::FastPass)  {
        return false;
    }

    const std::string& task_name = details.get("details", "task", "");
    if (task_name == m_config->get_theme() + "@Roguelike@Routing") {
        return true;
    }

    return false;
}

bool asst::RoguelikeRoutingTaskPlugin::_run() {
    LogTraceFunction;

    if (m_need_generate_map) {
        generate_map();
        m_need_generate_map = false;
    }
    select_route();

    return true;
}

bool asst::RoguelikeRoutingTaskPlugin::generate_map() {
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();
    std::size_t curr_col = 1;
    Rect roi = Task.get<MatchTaskInfo>(theme + "@Roguelike@MapNodeAnalyze")->roi;

    cv::Mat image = ctrler()->get_image();
    MultiTemplMatcher node_analyzer(image);
    node_analyzer.set_task_info(theme + "@Roguelike@MapNodeAnalyze");
    if (!node_analyzer.analyze()) {
        return false;
    }
    // ————————————————————————————————————————————————————————————————————————————————
    MultiTemplMatcher::ResultsVec results = node_analyzer.get_result();
    std::sort(results.begin(), results.end(), m_result_compare);

    for (const auto& [templ_name, rect, score] : results) {
        const RoguelikeNodeType type = m_templ_type_map.at(templ_name);
        const std::size_t node = m_map.create_and_insert_node(type, curr_col, rect.y).value();
        generate_edges(node, image, rect.x);
    }
    roi.x += m_column_offset;
    node_analyzer.set_roi(roi);
    while (node_analyzer.analyze()) {
        ++curr_col;
        results = node_analyzer.get_result();
        std::sort(results.begin(), results.end(), m_result_compare);

        for (const auto& [templ_name, rect, score] : results) {
            const RoguelikeNodeType type = m_templ_type_map.at(templ_name);
            const std::size_t node = m_map.create_and_insert_node(type, curr_col, rect.y).value();
            generate_edges(node, image, rect.x);
        }
        ctrler()->swipe(m_swipe_source, m_swipe_target, 200, true);
        sleep(300);
        image = ctrler()->get_image();
        node_analyzer.set_image(image);
    }

    ProcessTask(*this, { theme + "@Roguelike@MapExitThenContinue" }).run();

    return true;
}

bool asst::RoguelikeRoutingTaskPlugin::generate_edges(const std::size_t& curr,
                                                      const cv::Mat& image,
                                                      const int& curr_x) {
    LogTraceFunction;

    // double thresholdValue = 200;


    const std::size_t& curr_column = m_map.get_node_column(curr);
    if (curr_column == m_map.init_index) {
        return false;
    }
    if (curr_column == m_map.init_index + 1) {
        return m_map.add_edge(m_map.init_index, curr);
    }

    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
    cv::Mat binaryImage;
    cv::threshold(grayImage, binaryImage, 200, 255, cv::THRESH_BINARY);

    const int center_x = curr_x - (m_column_offset - m_node_width) / 2;
    const int curr_y = m_map.get_node_y(curr);
    cv::Rect roi(0, 0, 0, 0);

    const std::size_t& pre_col_begin = m_map.get_column_begin(curr_column - 1);
    const std::size_t& pre_col_end = m_map.get_column_end(curr_column - 1);
    for (std::size_t prev = pre_col_begin; prev < pre_col_end; ++prev) {
        const int prev_y = m_map.get_node_y(prev);
        const int center_y = (prev_y + curr_y + m_node_height) / 2;
        roi.x = center_x - m_roi_margin;
        roi.y = center_y - m_roi_margin;
        roi.width = m_roi_margin * 2;
        roi.height = m_roi_margin * 2;

        cv::Mat croppedImage = binaryImage(roi);
        std::vector<cv::Point> brightPoints;
        cv::findNonZero(croppedImage, brightPoints);

        if (brightPoints.empty()) {
            continue;
        }

        int positive = ranges::max_element(brightPoints, m_point_compare_positive)->y
                     - ranges::min_element(brightPoints, m_point_compare_positive)->y;

        int negative = ranges::min_element(brightPoints, m_point_compare_negative)->y
                     - ranges::max_element(brightPoints, m_point_compare_negative)->y;

        if (prev_y < curr_y && positive > negative + m_direction_threshold) {
            m_map.add_edge(prev, curr);
        }
        else if (prev_y > curr_y && positive < negative - m_direction_threshold) {
            m_map.add_edge(prev, curr);
        }
        else if (prev_y == curr_y && std::abs(positive - negative) < m_direction_threshold) {
            m_map.add_edge(prev, curr);
        }
    }
    // ————————————————————————————————————————————————————————————————————————————————
    if (curr > m_map.get_column_begin(curr_column)) {
        std::size_t prev = curr -1;
        roi.x = curr_x + m_node_width / 2 - m_roi_margin;
        roi.y = (m_map.get_node_y(prev) + m_node_height + m_nameplate_offset + curr_y) / 2 - m_roi_margin;
        cv::Mat croppedImage = binaryImage(roi);
        if (cv::countNonZero(croppedImage)) {
            m_map.add_edge(prev, curr);
            m_map.add_edge(curr, prev);
        }
    }
    return true;
}

void asst::RoguelikeRoutingTaskPlugin::select_route() {
    LogTraceFunction;

    std::size_t column = m_map.find_column(m_pos).value();
    const std::size_t& col_begin = m_map.get_column_begin(column + 1);
    const std::size_t& col_end = m_map.get_column_end(column + 1);
    for (std::size_t node = col_begin; node < col_end; ++node) {
        if (!m_map.get_node_refresh_times(node)) {
            refresh_then_update_node(node, column + 1);
        };
    }
    const std::size_t next = m_map.nspf(m_pos);
    if (m_map.get_node_cost(next) >= 1000) {
        m_map.clear();
        m_pos = 0;
        m_need_generate_map = true;
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@ExitThenAbandon");
        return;
    }
    const int next_x = m_origin_x + (column < 1 ? 0 : m_column_offset);
    const int next_y = m_map.get_node_y(next);
    ctrler()->click(Point(next_x + m_node_width / 2, next_y + m_node_height / 2));
    sleep(300);

    if (m_map.get_node_type(next) == RoguelikeNodeType::Encounter) {
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@StageEncounterEnter");
    }
    else if (m_map.get_node_type(next) == RoguelikeNodeType::RogueTrader) {
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@StageTraderEnter");
        m_map.clear();
        m_pos = 0;
        m_need_generate_map = true;
    }
    else {
        m_map.clear();
        m_pos = 0;
        m_need_generate_map = true;
        Task.set_task_base("Sarkaz@Roguelike@RoutingAction", "Sarkaz@Roguelike@ExitThenAbandon");
    }


}

void asst::RoguelikeRoutingTaskPlugin::refresh_then_update_node(const std::size_t& node, const std::size_t& column) {
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();
    int node_x = m_origin_x + (column <= 1 ? 0 : m_column_offset);
    int node_y = m_map.get_node_y(node);
    Point click_point = {node_x + m_node_width / 2,  node_y + m_node_height / 2};
    ctrler()->click(click_point);
    sleep(300);
    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@RefreshNode" }).run();
    m_map.set_node_refresh_times(node, m_map.get_node_refresh_times(node) + 1);
    // ————————————————————————————————————————————————————————————————————————————————
    Matcher node_analyzer(ctrler()->get_image());
    node_analyzer.set_task_info(theme + "@Roguelike@MapNodeAnalyze");
    node_analyzer.set_roi(Rect(node_x, node_y, m_node_width, m_node_height));
    if (node_analyzer.analyze()) {
       Matcher::Result result = node_analyzer.get_result();
       m_map.set_node_type(node, m_templ_type_map.at(result.templ_name));
    }
}
