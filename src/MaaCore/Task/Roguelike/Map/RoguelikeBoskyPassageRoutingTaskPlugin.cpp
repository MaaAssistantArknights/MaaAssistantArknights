#include "RoguelikeBoskyPassageRoutingTaskPlugin.h"

#include "Common/AsstTypes.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/ImageIo.h"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"

bool asst::RoguelikeBoskyPassageRoutingTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    if (m_config->get_theme() == RoguelikeTheme::JieGarden) {
        // ———————— 加载 BoskyPassage 配置 ————————
        const TaskPtr bosky_config = Task.get("JieGarden@RoguelikeRoutingConfig_BoskyPassage");
        m_bosky_config = bosky_config->special_params;

        // ———————— 选择导航策略 ————————
        if (m_config->get_mode() == RoguelikeMode::FindPlaytime) {
            m_bosky_routing_strategy = RoutingStrategy::FindPlaytime_JieGarden;
            int target = m_config->get_find_playTime_target();
            RoguelikeBoskyPassageMap::get_instance().set_target_subtype(static_cast<RoguelikeBoskySubNodeType>(target));
            Log.info(__FUNCTION__, "| FindPlaytime mode enabled with target:", target);
            return true;
        }

        m_bosky_routing_strategy = RoutingStrategy::BoskyPassage_JieGarden;
        return true;
    }

    return false;
}

void asst::RoguelikeBoskyPassageRoutingTaskPlugin::reset_in_run_variables()
{
    RoguelikeBoskyPassageMap::get_instance().reset();
}

bool asst::RoguelikeBoskyPassageRoutingTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    std::string task_name = details.get("details", "task", "");
    // Log.debug(__FUNCTION__, "| Checking task:", task_name); // 不太建议加这个，会在日志中大量出现

    // trigger 任务的名字可以为 "...@Roguelike@Routing_BoskyPassage-..." 的形式
    if (const size_t pos = task_name.find('-'); pos != std::string::npos) {
        task_name = task_name.substr(0, pos);
    }

    if (task_name == m_config->get_theme() + "@Roguelike@Routing_BoskyPassage") {
        return true;
    }

    return false;
}

bool asst::RoguelikeBoskyPassageRoutingTaskPlugin::_run()
{
    LogTraceFunction;

    Log.info(__FUNCTION__, "| Running with bosky_routing_strategy:", static_cast<int>(m_bosky_routing_strategy));

    switch (m_bosky_routing_strategy) {
    case RoutingStrategy::BoskyPassage_JieGarden: {
        bosky_update_map();
        const std::vector<RoguelikeNodeType> priority_order = get_bosky_passage_priority("Default");
        bosky_decide_and_click(priority_order);
        break;
    }
    case RoutingStrategy::FindPlaytime_JieGarden: {
        // 更新地图
        bosky_update_map();
        const std::vector<RoguelikeNodeType> priority_order = get_bosky_passage_priority("FindPlaytime");

        // 获取目标常乐节点子类型
        Log.info(
            __FUNCTION__,
            "| Looking for playtime subtype:",
            subtype2name(RoguelikeBoskyPassageMap::get_instance().get_target_subtype()));

        // 尝试找到目标节点，使用常乐节点优先的策略
        bosky_decide_and_click(priority_order);
        break;
    }
    default:
        break;
    }

    return true;
}

// ==================== JieGarden BoskyPassage 平面地图逻辑 ====================
void asst::RoguelikeBoskyPassageRoutingTaskPlugin::bosky_update_map()
{
    LogTraceFunction;

    Log.info(__FUNCTION__, "| updating bosky map");

    // 有时候从不期而遇出来可能会多点一下，点到剩余烛火，导致接下来的一次点击只会把窗口关掉，无法进入节点。而且还遮挡了部分节点
    // 能检测到意识回归的话就点一下边缘，把退出树洞的弹窗关掉
    ProcessTask(*this, { "JieGarden@Roguelike@LeaveBoskyPassageCheck" }).set_retry_times(0).run();

    cv::Mat image = ctrler()->get_image();
    if (image.empty()) {
        Log.error(__FUNCTION__, "| Failed to get image from controller");
        return;
    }

    MultiMatcher node_analyzer(image);
    node_analyzer.set_task_info("JieGarden@RoguelikeRoutingNodeAnalyze_BoskyPassage");
    if (!node_analyzer.analyze()) {
        Log.error(__FUNCTION__, "| no nodes are recognised");
        return;
    }

    MultiMatcher::ResultsVec match_results = node_analyzer.get_result();
    Log.info(__FUNCTION__, "| found", match_results.size(), "nodes");

    // 排序 靠左上优先
    sort_by_vertical_(match_results);

    const std::string& theme = m_config->get_theme();

#ifdef ASST_DEBUG
    cv::Mat image_draw = image.clone();
#endif

    // 处理每个识别到的节点
    for (const auto& [rect, score, templ_name] : match_results) {
        Log.debug(__FUNCTION__, "| analyzing node", templ_name, "at (", rect.x, ",", rect.y, ")");

        const RoguelikeNodeType type = RoguelikeMapInfo.templ2type(theme, templ_name);
        if (type == RoguelikeNodeType::Unknown) {
            Log.warn(__FUNCTION__, "| unknown template:", templ_name);
            continue;
        }

        // 检查是否为灰色节点
        const bool is_open = templ_name.find("Grey") == std::string::npos;

        auto idx = RoguelikeBoskyPassageMap::get_instance()
                       .ensure_node_from_pixel(rect.x, rect.y, m_bosky_config, is_open, type);

        if (idx.has_value()) {
            // 更新节点类型（防止类型不一致）
            RoguelikeBoskyPassageMap::get_instance().set_node_type(idx.value(), type);
            Log.debug(__FUNCTION__, "| updated node (", idx.value(), ") type: (", type2name(type), ")");
        }
        else {
            Log.warn(__FUNCTION__, "| failed to create/update node from pixel (", rect.x, ",", rect.y, ")");
        }

#ifdef ASST_DEBUG
        if (idx.has_value()) {
            cv::rectangle(image_draw, make_rect<cv::Rect>(rect), cv::Scalar(0, 0, 255), 2);
            cv::putText(
                image_draw,
                std::to_string(static_cast<int>(type)) + " (" +
                    std::to_string(RoguelikeBoskyPassageMap::get_instance().get_node_x(idx.value())) + ", " +
                    std::to_string(RoguelikeBoskyPassageMap::get_instance().get_node_y(idx.value())) + ")",
                cv::Point(rect.x, rect.y - 5),
                cv::FONT_HERSHEY_SIMPLEX,
                0.5,
                cv::Scalar(0, 0, 255),
                1);
        }
#endif
    }

#ifdef ASST_DEBUG
    utils::save_debug_image(
        image_draw,
        utils::path("debug") / "roguelikeMap",
        /*auto_clean=*/true,
        /*description=*/"bosky map draw",
        /*suffix=*/"draw");
#endif

    Log.info(__FUNCTION__, "| map updated with", RoguelikeBoskyPassageMap::get_instance().size(), "nodes");
}

void asst::RoguelikeBoskyPassageRoutingTaskPlugin::bosky_decide_and_click(
    const std::vector<RoguelikeNodeType>& priority_order)
{
    LogTraceFunction;

    Log.info(__FUNCTION__, "| deciding and clicking a bosky passage node");

    size_t chosen = 0;
    bool found = false;

    // 按优先级顺序查找可用的节点
    for (const auto& node_type : priority_order) {
        auto nodes_of_type = RoguelikeBoskyPassageMap::get_instance().get_open_unvisited_nodes(node_type);
        if (!nodes_of_type.empty()) {
            chosen = nodes_of_type.front();
            found = true;
            Log.debug(__FUNCTION__, "| found node of type (", type2name(node_type), ") with index (", chosen, ")");
            break;
        }
    }

    if (!found) {
        Log.info(__FUNCTION__, "| no open unvisited nodes available");
        Task.set_task_base("RoguelikeRoutingAction", "JieGarden@RoguelikeRoutingAction-LeaveBoskyPassage");
        return;
    }

    int gx = RoguelikeBoskyPassageMap::get_instance().get_node_x(chosen);
    int gy = RoguelikeBoskyPassageMap::get_instance().get_node_y(chosen);
    RoguelikeNodeType node_type = RoguelikeBoskyPassageMap::get_instance().get_node_type(chosen);

    Log.info(__FUNCTION__, "| chosen node:", chosen, "(", gx, ",", gy, ") type:", type2name(node_type));

    // 点击节点中心
    auto [px, py] = RoguelikeBoskyPassageMap::get_instance().get_node_pixel(
        chosen,
        m_bosky_config.origin_x,
        m_bosky_config.origin_y,
        m_bosky_config.column_offset,
        m_bosky_config.row_offset);

    if (px == -1 || py == -1) {
        Log.error(__FUNCTION__, "| Invalid pixel coordinates for node", chosen, ": (", px, ",", py, ")");
        return;
    }

    Point click_point(px + m_bosky_config.node_width / 2, py + m_bosky_config.node_height / 2);
    sleep(200);
    ctrler()->click(click_point);
    RoguelikeBoskyPassageMap::get_instance().set_visited(chosen);
    RoguelikeBoskyPassageMap::get_instance().set_curr_pos(chosen);

    // 发送节点类型到 WPF
    std::string node_type_name = type2name(node_type);
    auto node_info = basic_info_with_what("BoskyPassageNode");
    node_info["details"]["node_type"] = node_type_name;
    callback(AsstMsg::SubTaskExtraInfo, node_info);

    // 执行节点类型对应的任务
    const std::string& theme = m_config->get_theme();
    std::string node_name = type2name(node_type);

    const std::string node_task_name = theme + "@RoguelikeRoutingAction-Stage" + node_name + "Enter";
    // 设置 next
    Task.set_task_base("RoguelikeRoutingAction", node_task_name);
}

std::vector<asst::RoguelikeNodeType>
    asst::RoguelikeBoskyPassageRoutingTaskPlugin::get_bosky_passage_priority(const std::string& strategy)
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();
    const std::string config_name = theme + "@RoguelikeRouting-BoskyPassagePriority_" + strategy;

    const auto& task_info = Task.get<MatchTaskInfo>(config_name);
    if (!task_info) {
        Log.error(__FUNCTION__, "| priority config not found:", config_name);
        return {};
    }

    // 从 next 字段中读取优先级配置
    const auto& template_list = task_info->templ_names;
    if (template_list.empty()) {
        Log.warn(__FUNCTION__, "| Priority config is empty in:", config_name);
        return {};
    }

    // 从任务名称中解析节点类型
    auto priority_order_view = template_list | std::views::transform([&](const std::string& templ_name) {
                                   return RoguelikeMapInfo.templ2type(theme, templ_name);
                               });
    std::vector<RoguelikeNodeType> priority_order(priority_order_view.begin(), priority_order_view.end());

    Log.info(__FUNCTION__, "| Loaded", priority_order.size(), "node types from priority config");
    return priority_order;
}
