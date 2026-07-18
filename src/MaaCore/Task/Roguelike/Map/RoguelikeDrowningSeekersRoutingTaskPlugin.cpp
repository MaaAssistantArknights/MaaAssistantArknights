#include "RoguelikeDrowningSeekersRoutingTaskPlugin.h"

#include <climits>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

using namespace asst;

// ============================================================================
// 投资模式策略
// ============================================================================
namespace {

// 软避战代价：进入这些格 +1000（可被穿越，仅在无替代路线时才会走）
constexpr int kBattlePenalty = 1000;

bool is_battle_type(RoguelikeNodeType t)
{
    return t == RoguelikeNodeType::CombatOps || t == RoguelikeNodeType::EmergencyOps ||
           t == RoguelikeNodeType::FerociousPresage || t == RoguelikeNodeType::Unknown;
}

std::string format_map_ascii(const RoguelikeDrowningSeekersMapAnalyzer::Result& result)
{
    std::string output;
    auto connected = [&](int col, int row, int next_col, int next_row) {
        for (const auto& [a, b] : result.edges) {
            if ((a.first == col && a.second == row && b.first == next_col && b.second == next_row) ||
                (b.first == col && b.second == row && a.first == next_col && a.second == next_row)) {
                return true;
            }
        }
        return false;
    };

    for (int row = 0; row < result.rows; ++row) {
        for (int col = 0; col < result.cols; ++col) {
            char marker = ' ';
            for (const auto& cell : result.cells) {
                if (cell.col != col || cell.row != row) {
                    continue;
                }
                if (cell.col == result.player.first && cell.row == result.player.second) {
                    marker = '@';
                }
                else if (cell.kind == RoguelikeDrowningSeekersMapAnalyzer::CellKind::Object) {
                    marker = cell.type == RoguelikeNodeType::CombatOps ? 'C' : 'O';
                }
                else {
                    marker = '.';
                }
                break;
            }
            output += '[';
            output += marker;
            output += ']';
            if (col + 1 < result.cols) {
                output += connected(col, row, col + 1, row) ? "---" : "   ";
            }
        }
        if (row + 1 < result.rows) {
            output += '\n';
            std::string vertical(static_cast<size_t>(result.cols) * 6, ' ');
            for (int col = 0; col < result.cols; ++col) {
                if (connected(col, row, col, row + 1)) {
                    vertical[static_cast<size_t>(col) * 6 + 1] = '|';
                }
            }
            output += vertical;
            output += '\n';
        }
    }
    return output;
}

// 进入某格的代价：战斗格 +1000，其余 1（保证走最短跳数）
RoguelikeDrowningSeekersMap::CostFun make_soft_avoid_cost()
{
    return [](int /*col*/, int /*row*/, const RoguelikeDrowningSeekersMap::Cell& cell) -> int {
        return 1 + (is_battle_type(cell.type) ? kBattlePenalty : 0);
    };
}

} // namespace

DrowningSeekersRouteDecision DrowningSeekersInvestmentStrategy::decide(
    const RoguelikeDrowningSeekersMap& map,
    int action_points) const
{
    DrowningSeekersRouteDecision d;

    // 1. 行动力未知或过低 → 放弃
    if (action_points < 0) {
        d.action = DrowningSeekersRouteDecision::Action::Abandon;
        d.reason = "action_points_unknown";
        return d;
    }
    if (action_points <= m_abandon_ap) {
        d.action = DrowningSeekersRouteDecision::Action::Abandon;
        d.reason = "action_points_too_low";
        return d;
    }

    const auto cost_fun = make_soft_avoid_cost();
    const auto [pcol, prow] = map.player();

    auto plan_to = [&](int tcol, int trow) -> std::pair<int, std::vector<std::pair<int, int>>> {
        const auto path = map.shortest_path(tcol, trow, cost_fun);
        if (path.empty()) {
            return { INT_MAX, {} };
        }
        return { map.path_cost(tcol, trow, cost_fun), path };
    };

    // 2. 优先诡意行商：走向最近（代价最小）者
    auto traders = map.find_nodes(RoguelikeNodeType::RogueTrader, /*exclude_visited=*/true);
    {
        int best_cost = INT_MAX;
        std::vector<std::pair<int, int>> best_path;
        std::pair<int, int> best_target = { -1, -1 };
        for (const auto& [tc, tr] : traders) {
            auto [cost, path] = plan_to(tc, tr);
            if (path.size() >= 2 && cost < best_cost) {
                best_cost = cost;
                best_path = path;
                best_target = { tc, tr };
            }
        }
        if (!best_path.empty()) {
            d.action = DrowningSeekersRouteDecision::Action::Move;
            d.next = best_path[1]; // path[0] 是玩家格
            d.target = best_target;
            d.reason = "toward_rogue_trader";
            return d;
        }
    }

    // 3. 否则走「未知的诡秘」最多的路线（并列取代价小者）
    auto mysteries = map.find_nodes(RoguelikeNodeType::MysteriousPresage, /*exclude_visited=*/true);
    if (mysteries.empty() && traders.empty()) {
        // 4. 图中无行商也无诡秘 → 放弃
        d.action = DrowningSeekersRouteDecision::Action::Abandon;
        d.reason = "no_trader_no_mystery";
        return d;
    }

    auto count_mysteries_on_path = [&](const std::vector<std::pair<int, int>>& path) -> int {
        int cnt = 0;
        for (const auto& [c, r] : path) {
            if (map.cell(c, r).type == RoguelikeNodeType::MysteriousPresage) {
                ++cnt;
            }
        }
        return cnt;
    };

    int best_mystery_count = -1;
    int best_cost = INT_MAX;
    std::vector<std::pair<int, int>> best_path;
    std::pair<int, int> best_target = { -1, -1 };
    for (const auto& [tc, tr] : mysteries) {
        auto [cost, path] = plan_to(tc, tr);
        if (path.size() < 2) {
            continue;
        }
        const int mc = count_mysteries_on_path(path);
        if (mc > best_mystery_count || (mc == best_mystery_count && cost < best_cost)) {
            best_mystery_count = mc;
            best_cost = cost;
            best_path = path;
            best_target = { tc, tr };
        }
    }

    if (best_path.empty()) {
        d.action = DrowningSeekersRouteDecision::Action::Abandon;
        d.reason = "no_reachable_mystery";
        return d;
    }

    d.action = DrowningSeekersRouteDecision::Action::Move;
    d.next = best_path[1];
    d.target = best_target;
    d.reason = "toward_most_mysteries";
    return d;
}

// ============================================================================
// 插件
// ============================================================================
bool RoguelikeDrowningSeekersRoutingTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    if (m_config->get_theme() != RoguelikeTheme::DrowningSeekers) {
        return false;
    }

    const TaskPtr config_task = Task.get("DrowningSeekers@RoguelikeRoutingConfig");
    if (config_task && config_task->special_params.size() >= 4) {
        m_grid_step = config_task->special_params.at(0);
        m_nameplate_offset = config_task->special_params.at(1);
        m_abandon_ap = config_task->special_params.at(2);
        m_dry_run = config_task->special_params.at(3) != 0;
    }

    // 目前仅投资模式启用迷宫导航
    if (m_config->get_mode() == RoguelikeMode::Investment) {
        m_strategy = std::make_unique<DrowningSeekersInvestmentStrategy>(m_abandon_ap);
        return true;
    }

    return false;
}

void RoguelikeDrowningSeekersRoutingTaskPlugin::reset_in_run_variables()
{
    if (m_config->get_mode() == RoguelikeMode::Investment && !m_strategy) {
        m_strategy = std::make_unique<DrowningSeekersInvestmentStrategy>(m_abandon_ap);
    }
}

bool RoguelikeDrowningSeekersRoutingTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    std::string task_name = details.get("details", "task", "");

    // trigger 任务名可以为 "...@Roguelike@Routing-Investment" 的形式（截断 '-' 后缀）
    if (const size_t pos = task_name.find('-'); pos != std::string::npos) {
        task_name = task_name.substr(0, pos);
    }

    return task_name == m_config->get_theme() + "@Roguelike@Routing";
}

RoguelikeDrowningSeekersMapAnalyzer::Result RoguelikeDrowningSeekersRoutingTaskPlugin::recognize_map()
{
    // 1. 确保地图缩小：同一帧比较“＋”和“−”的置信率。
    //    游戏在最小缩放时仍会保留“−”按钮，因此不能以“−”消失作为退出条件。
    constexpr double zoom_threshold = 0.8;
    constexpr int zoom_max_clicks = 8;
    constexpr int zoom_wait_ms = 1000;
    constexpr const char* zoom_in_template = "DrowningSeekers@Roguelike@MapZoomIn.png";
    constexpr const char* zoom_out_template = "DrowningSeekers@Roguelike@MapZoomOut.png";
    const Rect zoom_roi { 0, 540, 120, 100 };

    for (int click_count = 0; click_count < zoom_max_clicks && !need_exit(); ++click_count) {
        const cv::Mat image = ctrler()->get_image();
        if (image.empty()) {
            Log.error(__FUNCTION__, "| Failed to get image while checking zoom controls");
            break;
        }

        Matcher zoom_in_matcher(image, zoom_roi);
        zoom_in_matcher.set_templ(zoom_in_template);
        zoom_in_matcher.set_threshold(0.0);
        const auto zoom_in = zoom_in_matcher.analyze();

        Matcher zoom_out_matcher(image, zoom_roi);
        zoom_out_matcher.set_templ(zoom_out_template);
        zoom_out_matcher.set_threshold(0.0);
        const auto zoom_out = zoom_out_matcher.analyze();

        const double zoom_in_score = zoom_in ? zoom_in->score : 0.0;
        const double zoom_out_score = zoom_out ? zoom_out->score : 0.0;
        Log.info(__FUNCTION__, "| zoom controls: in=", zoom_in_score, "out=", zoom_out_score);

        if (zoom_in_score > zoom_out_score || !zoom_out || zoom_out_score < zoom_threshold) {
            break;
        }

        const Point click_point {
            zoom_out->rect.x + zoom_out->rect.width / 2,
            zoom_out->rect.y + zoom_out->rect.height / 2
        };
        Log.info(__FUNCTION__, "| clicking zoom out at", click_point.x, click_point.y);
        ctrler()->click(click_point);
        sleep(zoom_wait_ms);
    }

    // 2. 从右上向左下滑动，固定视野
    ProcessTask(*this, { "DrowningSeekers@RoguelikeRouting-SwipeToCorner" }).run();
    sleep(300);

    // 3. 截图识别
    cv::Mat image = ctrler()->get_image();
    utils::save_debug_image(
        image,
        utils::path("debug") / "roguelikeMap",
        true,
        "drowningseekers recognition input",
        "input");
    RoguelikeDrowningSeekersMapAnalyzer analyzer(image);
    return analyzer.analyze();
}

RoguelikeDrowningSeekersMap
    RoguelikeDrowningSeekersRoutingTaskPlugin::build_map(const RoguelikeDrowningSeekersMapAnalyzer::Result& result)
{
    RoguelikeDrowningSeekersMap map;
    map.set_dimensions(result.cols, result.rows);
    for (const auto& c : result.cells) {
        RoguelikeDrowningSeekersMap::Cell cell;
        cell.kind = c.kind == RoguelikeDrowningSeekersMapAnalyzer::CellKind::Object
                        ? RoguelikeDrowningSeekersMap::CellKind::Object
                        : RoguelikeDrowningSeekersMap::CellKind::Road;
        cell.type = c.type;
        cell.center = c.center;
        cell.visited = c.visited;
        map.set_cell(c.col, c.row, cell);
    }
    for (const auto& [a, b] : result.edges) {
        map.add_edge(a.first, a.second, b.first, b.second);
    }
    map.set_player(result.player.first, result.player.second);
    return map;
}

int RoguelikeDrowningSeekersRoutingTaskPlugin::recognize_action_points()
{
    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("DrowningSeekers@Roguelike@ActionPointsRecognition");
    analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
    analyzer.set_use_char_model(true);
    if (!analyzer.analyze()) {
        return -1;
    }
    int val = 0;
    if (!utils::chars_to_number(analyzer.get_result().front().text, val)) {
        return -1;
    }
    return val;
}

void RoguelikeDrowningSeekersRoutingTaskPlugin::dump_recognition(
    const RoguelikeDrowningSeekersMapAnalyzer::Result& result,
    int action_points) const
{
    Log.info(
        "DrowningSeekersRouting | === recognition dump ===",
        "map",
        result.cols,
        "x",
        result.rows,
        "| player (",
        result.player.first,
        ",",
        result.player.second,
        ") | action_points",
        action_points);

    Log.info("  map legend: @=player C=combat O=object .=road [ ]=empty");
    for (int row = 0; row < result.rows; ++row) {
        const std::string prefix = "  row " + std::to_string(row) + ": ";
        std::string line = prefix;
        for (int col = 0; col < result.cols; ++col) {
            char marker = ' ';
            for (const auto& c : result.cells) {
                if (c.col != col || c.row != row) {
                    continue;
                }
                if (c.col == result.player.first && c.row == result.player.second) {
                    marker = '@';
                }
                else if (c.kind == RoguelikeDrowningSeekersMapAnalyzer::CellKind::Object) {
                    marker = c.type == RoguelikeNodeType::CombatOps ? 'C' : 'O';
                }
                else {
                    marker = '.';
                }
                break;
            }
            line += '[';
            line += marker;
            line += ']';
            if (col + 1 < result.cols) {
                bool connected = false;
                for (const auto& [a, b] : result.edges) {
                    connected = (a.first == col && a.second == row && b.first == col + 1 && b.second == row) ||
                                (b.first == col && b.second == row && a.first == col + 1 && a.second == row);
                    if (connected) {
                        break;
                    }
                }
                line += connected ? "---" : "   ";
            }
        }
        Log.info(line);

        if (row + 1 < result.rows) {
            std::string vertical(prefix.size() + static_cast<size_t>(result.cols) * 6, ' ');
            bool has_vertical = false;
            for (int col = 0; col < result.cols; ++col) {
                for (const auto& [a, b] : result.edges) {
                    const bool connected =
                        (a.first == col && a.second == row && b.first == col && b.second == row + 1) ||
                        (b.first == col && b.second == row && a.first == col && a.second == row + 1);
                    if (connected) {
                        vertical[prefix.size() + static_cast<size_t>(col) * 6 + 1] = '|';
                        has_vertical = true;
                        break;
                    }
                }
            }
            if (has_vertical) {
                Log.info(vertical);
            }
        }
    }

    for (const auto& c : result.cells) {
        Log.info(
            "  node (",
            c.col,
            ",",
            c.row,
            ") kind",
            static_cast<int>(c.kind),
            c.kind == RoguelikeDrowningSeekersMapAnalyzer::CellKind::Object ? type2name(c.type) : std::string("road"),
            c.visited ? "[visited]" : "",
            c.ocr_text.empty() ? "" : ("ocr=" + c.ocr_text));
    }
    for (const auto& [a, b] : result.edges) {
        Log.info("  edge (", a.first, ",", a.second, ")-(", b.first, ",", b.second, ")");
    }
}

void RoguelikeDrowningSeekersRoutingTaskPlugin::act_abandon(const std::string& reason)
{
    Log.info("DrowningSeekersRouting | abandon:", reason);
    callback(
        AsstMsg::SubTaskExtraInfo,
        [&] {
            auto info = basic_info_with_what("DrowningSeekersRoutingDecision");
            info["details"]["action"] = "abandon";
            info["details"]["reason"] = reason;
            return info;
        }());
    Task.set_task_base("RoguelikeRoutingAction", "DrowningSeekers@RoguelikeRoutingAction-ExitThenAbandon");
}

void RoguelikeDrowningSeekersRoutingTaskPlugin::act_move(
    const RoguelikeDrowningSeekersMap& map,
    const DrowningSeekersRouteDecision& decision)
{
    const auto [ncol, nrow] = decision.next;
    const RoguelikeDrowningSeekersMap::Cell& target_cell = map.cell(ncol, nrow);

    Log.info(
        "DrowningSeekersRouting | move to (",
        ncol,
        ",",
        nrow,
        ") type",
        type2name(target_cell.type),
        "| target (",
        decision.target.first,
        ",",
        decision.target.second,
        ") | reason",
        decision.reason);

    callback(
        AsstMsg::SubTaskExtraInfo,
        [&] {
            auto info = basic_info_with_what("DrowningSeekersRoutingDecision");
            info["details"]["action"] = "move";
            info["details"]["reason"] = decision.reason;
            info["details"]["next_col"] = ncol;
            info["details"]["next_row"] = nrow;
            info["details"]["next_type"] = type2name(target_cell.type);
            return info;
        }());

    // 点击相邻格中心
    ctrler()->click(Point(target_cell.center.x, target_cell.center.y));
    sleep(300);

    // 衔接节点进入流程：无论何种类型，统一交给 EnterNode 逐模板尝试进入弹窗；
    // road/空节点无弹窗时经 Stop 优雅返回 Stages 循环重新识别。
    Task.set_task_base("RoguelikeRoutingAction", "DrowningSeekers@RoguelikeRoutingAction-EnterNode");
}

bool RoguelikeDrowningSeekersRoutingTaskPlugin::_run()
{
    LogTraceFunction;

    if (!m_strategy) {
        return false;
    }

    const RoguelikeDrowningSeekersMapAnalyzer::Result result = recognize_map();
    const int action_points = recognize_action_points();

    if (!result.valid) {
        Log.error("DrowningSeekersRouting | map recognition failed, abandoning");
        act_abandon("recognition_failed");
        return true;
    }

    dump_recognition(result, action_points);

    // 回传识别摘要给 WPF（便于调试观测）
    auto recog_info = basic_info_with_what("DrowningSeekersMapRecognition");
    recog_info["details"]["cols"] = result.cols;
    recog_info["details"]["rows"] = result.rows;
    recog_info["details"]["nodes"] = static_cast<int>(result.cells.size());
    recog_info["details"]["edges"] = static_cast<int>(result.edges.size());
    recog_info["details"]["player_col"] = result.player.first;
    recog_info["details"]["player_row"] = result.player.second;
    recog_info["details"]["action_points"] = action_points;
    recog_info["details"]["map_ascii"] = format_map_ascii(result);
    callback(AsstMsg::SubTaskExtraInfo, recog_info);

    if (m_dry_run) {
        // 仅识别，安全退出以便人工核对识别正确性（M1）。
        act_abandon("dry_run");
        return true;
    }

    // 策略决策 → 移动 / 放弃
    const RoguelikeDrowningSeekersMap map = build_map(result);
    const DrowningSeekersRouteDecision decision = m_strategy->decide(map, action_points);

    if (decision.action == DrowningSeekersRouteDecision::Action::Move && map.in_bounds(decision.next.first, decision.next.second)) {
        act_move(map, decision);
    }
    else {
        act_abandon(decision.reason.empty() ? "no_move" : decision.reason);
    }
    return true;
}
