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
        const auto mode = m_config->get_mode();
        if (mode == RoguelikeMode::FindPlaytime) {
            m_bosky_routing_strategy = RoutingStrategy::FindPlaytime_JieGarden;
            int target = m_config->get_find_playTime_target();
            RoguelikeBoskyPassageMap::get_instance().set_target_subtype(static_cast<RoguelikeBoskySubNodeType>(target));
            Log.info(
                __FUNCTION__,
                "| FindPlaytime-like mode enabled with target:",
                target,
                "| mode:",
                static_cast<int>(mode));
            return true;
        }
        if (mode == RoguelikeMode::CoppersRecast) {
            // 投币重铸同样按常乐优先在树洞里找节点；但常乐的子类型（掷地有声/种因得果/三缺一）
            // 要进节点后由不期而遇识别，路由阶段分不出来，所以不设目标子类型，
            // 目标判定在 RoguelikeStageEncounterTaskPlugin 里做
            m_bosky_routing_strategy = RoutingStrategy::FindPlaytime_JieGarden;
            LogInfo << __FUNCTION__ << "| coppers recast mode enabled, playtime-first routing";
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
    m_map_initialized = false;
    m_reset_map_pending = false;
    m_last_clicked.reset();
    m_click_fail_count.clear();
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

    // 主地图上点击树洞节点 = 即将进入一个新的树洞。地图每次进入都会重新随机，
    // 而单例里的旧节点不会自己消失（reset 只在 Roguelike@StartExplore 触发）。
    if (task_name == m_config->get_theme() + "@Roguelike@StageBoskyPassage") {
        m_reset_map_pending = true;
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

    if (!m_map_initialized || m_reset_map_pending) {
        // 一张树洞地图 = 一份节点状态。插件随任务重建，所以重开任务时这里也会清一次，
        // 不会沿用上一轮残留的「已访问/开放」状态（否则会跳过真正的目标节点去点别的格子）。
        RoguelikeBoskyPassageMap::get_instance().reset();
        m_click_fail_count.clear();
        m_last_clicked.reset();
        m_map_initialized = true;
        m_reset_map_pending = false;
        Log.info(__FUNCTION__, "| fresh bosky map, node state cleared");
    }

    // 上一次点击到现在过了多久；点击后地图很快又出现 = 那次点击没进节点
    std::optional<size_t> previous_click = m_last_clicked;
    long long previous_click_ms = 0;
    if (previous_click.has_value()) {
        m_last_clicked.reset();
        previous_click_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - m_last_click_time)
                               .count();
    }

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

    // 每次识别前清标记，识别完只有当前画面里出现的节点才是 seen
    RoguelikeBoskyPassageMap::get_instance().clear_seen_flags();

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

    if (previous_click.has_value()) {
        const size_t idx = previous_click.value();
        auto& fails = m_click_fail_count[idx];
        const bool still_open = RoguelikeBoskyPassageMap::get_instance().get_node_open(idx);
        if (previous_click_ms < 4000 && still_open && fails < 3) {
            // 点完立刻回到选点 = 这一下没把节点打开（没点中 / 被弹窗吃掉），撤销 visited 重试，
            // 否则它会当成已完成，退而求其次去点优先级更低、还压在它上面一格的节点。
            ++fails;
            RoguelikeBoskyPassageMap::get_instance().set_unvisited(idx);
            Log.warn(
                __FUNCTION__,
                "| click on node",
                idx,
                "did not open it after",
                previous_click_ms,
                "ms, retry",
                fails,
                "/3");
        }
        else {
            m_click_fail_count.erase(idx);
        }
    }

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
        if (nodes_of_type.empty()) {
            continue;
        }

        {
            std::string cells;
            for (const size_t idx : nodes_of_type) {
                cells += "(" + std::to_string(RoguelikeBoskyPassageMap::get_instance().get_node_x(idx)) + "," +
                         std::to_string(RoguelikeBoskyPassageMap::get_instance().get_node_y(idx)) + ")";
            }
            Log.info(__FUNCTION__, "| candidates of type", type2name(node_type), ":", cells);
        }

        // 只点本次截图里真的认到的节点：认不到说明它不在当前画面里（旧布局残留或已滚出视野），
        // 按格子反算的坐标未必还有节点
        std::vector<size_t> seen_nodes;
        seen_nodes.reserve(nodes_of_type.size());
        for (const size_t idx : nodes_of_type) {
            if (RoguelikeBoskyPassageMap::get_instance().get_node_seen(idx)) {
                seen_nodes.emplace_back(idx);
            }
        }
        if (!seen_nodes.empty()) {
            nodes_of_type = std::move(seen_nodes);
        }
        else {
            Log.warn(__FUNCTION__, "| no node of type (", type2name(node_type), ") is visible in current screen");
        }

        chosen = nodes_of_type.front();
        found = true;
        Log.debug(__FUNCTION__, "| found node of type (", type2name(node_type), ") with index (", chosen, ")");
        break;
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

    // 点击节点中心：直接用本次截图里识别到的图标位置（树洞地图固定，二者等价；
    // 用实测位置可以避免节点登记与格子不一致时点偏）
    auto [px, py] = RoguelikeBoskyPassageMap::get_instance().get_node_click_point(chosen, m_bosky_config);

    if (px == -1 || py == -1) {
        Log.error(__FUNCTION__, "| Invalid pixel coordinates for node", chosen, ": (", px, ",", py, ")");
        return;
    }

    Point click_point(px, py);
    Log.info(
        __FUNCTION__,
        "| click at (",
        px,
        ",",
        py,
        ") node index:",
        chosen,
        "| seen in current screen:",
        RoguelikeBoskyPassageMap::get_instance().get_node_seen(chosen));
    sleep(200);
    ctrler()->click(click_point);
    RoguelikeBoskyPassageMap::get_instance().set_visited(chosen);
    RoguelikeBoskyPassageMap::get_instance().set_curr_pos(chosen);
    m_last_clicked = chosen;
    m_last_click_time = std::chrono::steady_clock::now();

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
