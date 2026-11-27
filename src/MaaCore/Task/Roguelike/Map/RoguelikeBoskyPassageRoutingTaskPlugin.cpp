#include <algorithm>
#include <chrono>
#include <string_view>
#include <unordered_map>

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
#include "Vision/OCRer.h"

namespace
{
// 检查类 ProcessTask 的兜底：永远命中、什么都不做，详见资源里的 Doc
constexpr std::string_view ProbeFallbackTask = "JieGarden@Roguelike@ProbeFallback";

// 只认常乐节点的轻量扫描任务：模板只保留 JieGarden@RoguelikeMapNode-Playtime.png，
// roi 与其他树洞扫描一致。全量扫 12 个模板实测 7 秒，单模板约 0.6 秒。
constexpr std::string_view CoppersRecastNodeScanTask = "JieGarden@Roguelike@CoppersRecast-NodeScan";

// 常乐节点详情面板：点开节点后右侧写着「可以押上源石锭和令掷币取乐」，代理人名就在这句话里
constexpr std::string_view PlaytimeLingTask = "JieGarden@Roguelike@PlaytimeDetailLing";
constexpr std::string_view PlaytimeShuTask = "JieGarden@Roguelike@PlaytimeDetailShu";
constexpr std::string_view PlaytimeNianTask = "JieGarden@Roguelike@PlaytimeDetailNian";

// 点开节点到详情面板画出来大约 300ms，留点余量再截图
constexpr int PlaytimeDetailDelayMs = 500;

// 收起详情面板用的空白点：地图最左一列节点在 x=188 起，这个位置在列外、也不压底部按钮栏
constexpr int PlaytimeBlankX = 150;
constexpr int PlaytimeBlankY = 620;
// 点空白后面板收起大约 200ms
constexpr int PlaytimeBlankDelayMs = 350;

// 详情面板是否还开着（面板正文独有，地图节点铭牌上没有）
constexpr std::string_view PlaytimePanelTask = "JieGarden@Roguelike@PlaytimePanelPresent";

// 详情面板正文所在的区域：和资源里 PlaytimeDetail* 的 roi 保持一致
constexpr int PlaytimePanelRoiX = 820;
constexpr int PlaytimePanelRoiY = 190;
constexpr int PlaytimePanelRoiW = 440;
constexpr int PlaytimePanelRoiH = 280;

// 常乐节点在地图上只有一种图标，认不出是令/黍/年；进节点认过事件之后才知道子类型。
// 挑节点时按「已知是令」> 「还没进过」> 「认出来不是令」排序。
int playtime_subtype_rank(asst::RoguelikeBoskySubNodeType subtype)
{
    switch (subtype) {
    case asst::RoguelikeBoskySubNodeType::Ling:
        return 0;
    case asst::RoguelikeBoskySubNodeType::Unknown:
        return 1;
    default:
        return 2;
    }
}
}

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
            // 末尾的 build 标记用来核对手机上跑的到底是不是新包：装完在日志里搜这行，
            // 标记没变就说明 APK 没真正覆盖安装（旧包 versionCode 恒为 1，安装器会跳过）
            LogInfo << __FUNCTION__ << "| coppers recast build 1108a, playtime-first routing";
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
    m_last_probed_playtime.reset();
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
        m_playtime_seen_last_scan = false;
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
    // 能检测到意识回归的话就点一下边缘，把退出树洞的弹窗关掉。
    // 这是一次「检查」，没命中是常态；末尾挂个占位任务让流程走成功分支，
    // 否则每次更新地图都会冒一条 SubTaskError，客户端攒够几条就弹「任务失败」成就。
    ProcessTask(*this, { "JieGarden@Roguelike@LeaveBoskyPassageCheck", std::string(ProbeFallbackTask) })
        .set_retry_times(0)
        .run();

    cv::Mat image = ctrler()->get_image();
    if (image.empty()) {
        Log.error(__FUNCTION__, "| Failed to get image from controller");
        return;
    }

    // 每次识别前清标记，识别完只有当前画面里出现的节点才是 seen
    RoguelikeBoskyPassageMap::get_instance().clear_seen_flags();

    auto scan = [&image](std::string_view task) -> MultiMatcher::ResultsVec {
        MultiMatcher analyzer(image);
        analyzer.set_task_info(std::string(task));
        if (!analyzer.analyze()) {
            return {};
        }
        return analyzer.get_result();
    };

    // 全量扫一次地图要跑 12 个节点模板，1280x575 的 roi 下实测 7 秒；
    // 而投币循环里每次回地图要找的只有常乐节点。
    // 上一轮扫到过常乐，就先用「只认常乐」的单模板扫一遍（约 0.6 秒），
    // 扫不到（布局变了 / 已经不在这个界面）再退回全量，兜底逻辑不变。
    MultiMatcher::ResultsVec match_results;
    bool light_scan = m_playtime_seen_last_scan &&
                      m_bosky_routing_strategy == RoutingStrategy::FindPlaytime_JieGarden;
    if (light_scan) {
        const auto started = std::chrono::steady_clock::now();
        match_results = scan(CoppersRecastNodeScanTask);
        Log.info(
            __FUNCTION__,
            "| playtime-only scan:",
            match_results.size(),
            "nodes in",
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started)
                .count(),
            "ms");
        if (match_results.empty()) {
            Log.info(__FUNCTION__, "| no playtime in this scan, fall back to the full map scan");
            light_scan = false;
        }
    }
    if (!light_scan) {
        match_results = scan("JieGarden@RoguelikeRoutingNodeAnalyze_BoskyPassage");
    }

    if (match_results.empty()) {
        Log.error(__FUNCTION__, "| no nodes are recognised");
        return;
    }

    // 下一轮要不要用单模板扫，取决于这一轮有没有真扫到常乐
    m_playtime_seen_last_scan =
        std::any_of(match_results.cbegin(), match_results.cend(), [](const MultiMatcher::Result& result) {
            return result.templ_name.find("Playtime") != std::string::npos;
        });
    Log.info(__FUNCTION__, "| found", match_results.size(), "nodes", light_scan ? "(playtime-only)" : "(full)");

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
        // 按格子反算的坐标未必还有节点。以前这里认不到还会照着旧坐标点，容易点到别的格子上。
        std::vector<size_t> seen_nodes;
        seen_nodes.reserve(nodes_of_type.size());
        for (const size_t idx : nodes_of_type) {
            if (RoguelikeBoskyPassageMap::get_instance().get_node_seen(idx)) {
                seen_nodes.emplace_back(idx);
            }
        }
        if (seen_nodes.empty()) {
            Log.warn(__FUNCTION__, "| no node of type (", type2name(node_type), ") is visible in current screen");
            continue;
        }
        nodes_of_type = std::move(seen_nodes);

        // 常乐节点：图标分不出是令/黍/年，先逐个点开看详情面板，把代理人名记到节点上，
        // 再按「已确认是令」> 「还没探过」> 「认出来不是令」排序。
        // 投币重铸要反复进同一个常乐节点，认准了就别再换。
        if (node_type == RoguelikeNodeType::Playtime) {
            identify_playtime_nodes(nodes_of_type);

            std::stable_sort(nodes_of_type.begin(), nodes_of_type.end(), [](size_t lhs, size_t rhs) {
                return playtime_subtype_rank(RoguelikeBoskyPassageMap::get_instance().get_node_subtype(lhs)) <
                       playtime_subtype_rank(RoguelikeBoskyPassageMap::get_instance().get_node_subtype(rhs));
            });
            for (const size_t idx : nodes_of_type) {
                Log.info(
                    __FUNCTION__,
                    "| playtime candidate node",
                    idx,
                    "type:",
                    subtype2name(RoguelikeBoskyPassageMap::get_instance().get_node_subtype(idx)));
            }
            const size_t best = nodes_of_type.front();
            Log.info(
                __FUNCTION__,
                "| playtime candidates ranked, best node:",
                best,
                "subtype:",
                subtype2name(RoguelikeBoskyPassageMap::get_instance().get_node_subtype(best)));
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
    // 刚为了看详情点开过它的话，详情面板已经停在这个节点上，不用再点一次：
    // 重复点同一个节点会被当成取消选中，反而进不去
    const bool detail_already_on = m_last_probed_playtime.has_value() && m_last_probed_playtime.value() == chosen;
    Log.info(
        __FUNCTION__,
        "| click at (",
        px,
        ",",
        py,
        ") node index:",
        chosen,
        "| seen in current screen:",
        RoguelikeBoskyPassageMap::get_instance().get_node_seen(chosen),
        "| detail panel already on it:",
        detail_already_on);
    if (!detail_already_on) {
        sleep(200);
        ctrler()->click(click_point);
    }
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

void asst::RoguelikeBoskyPassageRoutingTaskPlugin::identify_playtime_nodes(const std::vector<size_t>& nodes)
{
    LogTraceFunction;

    // 只记这一次运行里最后点开的那个；跨次的详情状态不可靠
    m_last_probed_playtime.reset();

    for (const size_t idx : nodes) {
        const auto known = RoguelikeBoskyPassageMap::get_instance().get_node_subtype(idx);
        if (known != RoguelikeBoskySubNodeType::Unknown) {
            // 之前已经探过（或者进节点后由不期而遇认过），不用再点开
            continue;
        }

        auto [px, py] = RoguelikeBoskyPassageMap::get_instance().get_node_click_point(idx, m_bosky_config);
        if (px == -1 || py == -1) {
            Log.warn(__FUNCTION__, "| invalid pixel coordinate for playtime node", idx);
            continue;
        }

        Log.info(__FUNCTION__, "| open playtime node", idx, "at (", px, ",", py, ") to read its detail panel");
        ctrler()->click(Point(px, py));
        m_last_probed_playtime = idx;
        sleep(PlaytimeDetailDelayMs);

        const auto subtype = probe_playtime_subtype();
        RoguelikeBoskyPassageMap::get_instance().set_node_subtype(idx, subtype);
        Log.info(__FUNCTION__, "| playtime node", idx, "subtype:", subtype2name(subtype));

        if (subtype == RoguelikeBoskySubNodeType::Ling) {
            // 找到了：详情面板正停在这个节点上，后面直接用，不用再点一次
            // （重复点同一个节点会取消选中，反而点不动「出发」）
            break;
        }

        // 不是令：点一下地图空白处收起右侧详情。
        // 面板会盖住右边几列节点，不收起来的话下一次点击会打在面板上，点不中目标节点。
        Log.info(__FUNCTION__, "| close the detail panel before probing the next node");
        close_playtime_detail();
        m_last_probed_playtime.reset();
    }
}

void asst::RoguelikeBoskyPassageRoutingTaskPlugin::close_playtime_detail()
{
    for (int attempt = 1; attempt <= 2; ++attempt) {
        ctrler()->click(Point(PlaytimeBlankX, PlaytimeBlankY));
        sleep(PlaytimeBlankDelayMs);

        // 面板收起来才算数：没点掉的话下一次点击会落在面板上，
        // 读到的还是上一个节点的文案，会把两个节点标成同一个代理人
        ProcessTask check(*this, { std::string(PlaytimePanelTask), std::string(ProbeFallbackTask) });
        check.set_retry_times(0);
        if (!check.run() || std::string_view(check.get_last_task_name()) != PlaytimePanelTask) {
            return;
        }
        Log.warn(__FUNCTION__, "| detail panel is still open after clicking blank, retry", attempt, "/2");
    }
    Log.warn(__FUNCTION__, "| cannot close the detail panel, the next probe may read the same panel");
}

asst::RoguelikeBoskySubNodeType asst::RoguelikeBoskyPassageRoutingTaskPlugin::probe_playtime_subtype()
{
    // 末尾挂一个永远命中的占位任务：三个都没认出来时流程走占位、返回成功，
    // 就不会有 SubTaskError 冒到客户端去（那会被当成任务失败）
    ProcessTask probe(
        *this,
        { std::string(PlaytimeLingTask),
          std::string(PlaytimeShuTask),
          std::string(PlaytimeNianTask),
          std::string(ProbeFallbackTask) });
    probe.set_retry_times(0);
    if (!probe.run()) {
        return RoguelikeBoskySubNodeType::Unknown;
    }

    const std::string& last = probe.get_last_task_name();
    if (last == PlaytimeLingTask) {
        return RoguelikeBoskySubNodeType::Ling;
    }
    if (last == PlaytimeShuTask) {
        return RoguelikeBoskySubNodeType::Shu;
    }
    if (last == PlaytimeNianTask) {
        return RoguelikeBoskySubNodeType::Nian;
    }

    // 三个都没认出来：把面板上的原始文字打进日志，方便对照着调 roi / 关键词
    OCRer ocr(ctrler()->get_image());
    ocr.set_roi(Rect(PlaytimePanelRoiX, PlaytimePanelRoiY, PlaytimePanelRoiW, PlaytimePanelRoiH));
    const auto lines = ocr.analyze();
    if (!lines) {
        Log.warn(__FUNCTION__, "| detail panel has no readable text");
        return RoguelikeBoskySubNodeType::Unknown;
    }
    for (const auto& line : *lines) {
        Log.info(__FUNCTION__, "| panel text:", line.text);
    }
    return RoguelikeBoskySubNodeType::Unknown;
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
