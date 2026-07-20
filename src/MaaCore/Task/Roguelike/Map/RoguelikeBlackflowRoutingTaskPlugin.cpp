#include "RoguelikeBlackflowRoutingTaskPlugin.h"

#include <algorithm>
#include <climits>
#include <optional>
#include <string_view>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

using namespace asst;

// ============================================================================
// 工具
// ============================================================================
namespace {

// 徒步跋涉：不是加工品，永远是面板第一张卡
constexpr const char* kWalkName = "徒步跋涉";

// 加工品面板卡片几何（1280x720）：名称在卡片右下；
// "剩余N次"徽章中心在名称中心上方约 90px；"装载中"标签在名称中心上方约 25px。
constexpr int kBadgeWindowMin = 40;
constexpr int kBadgeWindowMax = 150;
constexpr int kLoadedWindowMin = 2;
constexpr int kLoadedWindowMax = 60;
constexpr int kCardClickX = 1063; // 卡片内名称行左侧空白处，避开文字

constexpr int kMaxPanelScrolls = 3;
constexpr int kMaxConsecutiveFailures = 3;

// 保留当前工作区的节点分类入口：地图分析器给出节点类型后，优先进入对应的
// 节点流程，避免再次让所有入口模板竞争同一个确认窗。
const char* known_node_route_action(RoguelikeNodeType type)
{
    switch (type) {
    case RoguelikeNodeType::CombatOps:
        return "Blackflow@RoguelikeRoutingAction-StageCombatOpsEnter";
    case RoguelikeNodeType::EmergencyOps:
        return "Blackflow@RoguelikeRoutingAction-StageEmergencyOpsEnter";
    case RoguelikeNodeType::DreadfulFoe:
        return "Blackflow@RoguelikeRoutingAction-StageDreadfulFoeEnter";
    case RoguelikeNodeType::Encounter:
        return "Blackflow@RoguelikeRoutingAction-StageEncounterEnter";
    case RoguelikeNodeType::Boons:
        return "Blackflow@RoguelikeRoutingAction-StageBoonsEnter";
    case RoguelikeNodeType::SafeHouse:
        return "Blackflow@RoguelikeRoutingAction-StageSafeHouseEnter";
    case RoguelikeNodeType::BoskyPassage:
        return "Blackflow@RoguelikeRoutingAction-StageBoskyPassageEnter";
    case RoguelikeNodeType::FaceOff:
        return "Blackflow@RoguelikeRoutingAction-StageConfrontationEnter";
    case RoguelikeNodeType::RogueTrader:
        return "Blackflow@RoguelikeRoutingAction-StageTraderEnter";
    case RoguelikeNodeType::LostAndFound:
        return "Blackflow@RoguelikeRoutingAction-StageWindAndRainEnter";
    case RoguelikeNodeType::PathEnd:
        return "Blackflow@RoguelikeRoutingAction-StageFinalEnter";
    case RoguelikeNodeType::HiddenTrader:
        return "Blackflow@RoguelikeRoutingAction-StageScrapShopEnter";
    case RoguelikeNodeType::EmergencyAid:
        return "Blackflow@RoguelikeRoutingAction-StageEmployEnter";
    case RoguelikeNodeType::MysteriousPresage:
        return "Blackflow@RoguelikeRoutingAction-StageMysteriousPresageEnter";
    case RoguelikeNodeType::FerociousPresage:
        return "Blackflow@RoguelikeRoutingAction-StageFerociousPresageEnter";
    default:
        return nullptr;
    }
}

// 解析"剩余N次"徽章
std::optional<int> parse_uses_badge(const std::string& text)
{
    static const std::string prefix = "剩余";
    const size_t pos = text.find(prefix);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    size_t i = pos + prefix.size();
    int value = 0;
    bool any_digit = false;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
        value = value * 10 + (text[i] - '0');
        ++i;
        any_digit = true;
    }
    if (!any_digit) {
        return std::nullopt;
    }
    return value;
}

std::string_view node_marker(const RoguelikeBlackflowMapAnalyzer::Cell& cell, bool is_player)
{
    if (is_player) {
        return "我";
    }
    if (cell.kind == RoguelikeBlackflowMapAnalyzer::CellKind::Road) {
        return "路";
    }
    if (cell.kind != RoguelikeBlackflowMapAnalyzer::CellKind::Object) {
        return "空";
    }

    switch (cell.type) {
    case RoguelikeNodeType::CombatOps:
        return "战";
    case RoguelikeNodeType::EmergencyOps:
        return "急";
    case RoguelikeNodeType::DreadfulFoe:
        return "敌";
    case RoguelikeNodeType::Encounter:
        return "遇";
    case RoguelikeNodeType::Boons:
        return "赐";
    case RoguelikeNodeType::SafeHouse:
        return "安";
    case RoguelikeNodeType::Recreation:
        return "休";
    case RoguelikeNodeType::RogueTrader:
        return "商";
    case RoguelikeNodeType::LostAndFound:
        return "失";
    case RoguelikeNodeType::Scout:
        return "探";
    case RoguelikeNodeType::BoskyPassage:
        return "林";
    case RoguelikeNodeType::MysteriousPresage:
        return "谜";
    case RoguelikeNodeType::FerociousPresage:
        return "凶";
    case RoguelikeNodeType::FaceOff:
        return "决";
    case RoguelikeNodeType::PathEnd:
        return "终";
    case RoguelikeNodeType::PathLane:
        return "径";
    case RoguelikeNodeType::HiddenTrader:
        return "隐";
    case RoguelikeNodeType::EmergencyAid:
        return "援";
    case RoguelikeNodeType::WindingPassage:
        return "曲";
    case RoguelikeNodeType::VantagePoint:
        return "瞰";
    case RoguelikeNodeType::ResidentStronghold:
        return "居";
    default:
        return "未";
    }
}

std::string format_map_ascii(const RoguelikeBlackflowMapAnalyzer::Result& result)
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
            const RoguelikeBlackflowMapAnalyzer::Cell* cell_at = nullptr;
            for (const auto& cell : result.cells) {
                if (cell.col != col || cell.row != row) {
                    continue;
                }
                cell_at = &cell;
                break;
            }
            if (cell_at != nullptr) {
                output += node_marker(*cell_at, cell_at->col == result.player.first && cell_at->row == result.player.second);
            }
            else {
                output += "空";
            }
            if (col + 1 < result.cols) {
                output += connected(col, row, col + 1, row) ? "－" : "　";
            }
        }
        if (row + 1 < result.rows) {
            output += '\n';
            std::string vertical;
            vertical.reserve(static_cast<size_t>(result.cols) * 3);
            for (int col = 0; col < result.cols; ++col) {
                vertical += connected(col, row, col, row + 1) ? "｜" : "　";
                if (col + 1 < result.cols) {
                    vertical += "　";
                }
            }
            output += vertical;
            output += '\n';
        }
    }
    return output;
}

// 路线仅显示每一步的方向：正八方向使用箭头，非相邻移动（例如喷气背包）使用星号。
std::string route_to_string(
    const drowning_seekers::PlannerResult& planned,
    const drowning_seekers::PlannerMap& map)
{
    std::string out;
    int current = map.player;
    for (const auto& action : planned.actions) {
        const int current_col = current >= 0 && map.cols > 0 ? current % map.cols : -1;
        const int current_row = current >= 0 && map.cols > 0 ? current / map.cols : -1;
        const int target_col = action.target >= 0 && map.cols > 0 ? action.target % map.cols : -1;
        const int target_row = action.target >= 0 && map.cols > 0 ? action.target / map.cols : -1;
        const int dc = target_col - current_col;
        const int dr = target_row - current_row;

        const char* marker = "★";
        if (dc >= -1 && dc <= 1 && dr >= -1 && dr <= 1 && (dc != 0 || dr != 0)) {
            static constexpr const char* kDirections[3][3] = {
                { "↖", "↑", "↗" },
                { "←", "", "→" },
                { "↙", "↓", "↘" },
            };
            marker = kDirections[dr + 1][dc + 1];
        }
        out += marker;

        current = action.target;
        if (current >= 0 && current < static_cast<int>(map.cells.size())) {
            const int twin = map.cells[static_cast<size_t>(current)].teleport_twin;
            if (twin >= 0) {
                current = twin;
            }
        }
    }
    return out;
}

} // namespace

// ============================================================================
// 生命周期
// ============================================================================
bool RoguelikeBlackflowRoutingTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    if (m_config->get_theme() != RoguelikeTheme::Blackflow) {
        return false;
    }

    const TaskPtr config_task = Task.get("Blackflow@RoguelikeRoutingConfig");
    if (config_task && config_task->special_params.size() >= 4) {
        m_grid_step = config_task->special_params.at(0);
        m_nameplate_offset = config_task->special_params.at(1);
        m_abandon_ap = config_task->special_params.at(2);
        m_dry_run = config_task->special_params.at(3) != 0;
    }

    // 该模式有策略档案才启用迷宫导航（resource/roguelike/Blackflow/routing.json 的 modeStrategies）
    m_profile = BlackflowRoutingInfo.strategy_for_mode(static_cast<int>(m_config->get_mode()));
    return m_profile != nullptr;
}

void RoguelikeBlackflowRoutingTaskPlugin::reset_in_run_variables()
{
    m_consecutive_failures = 0;
    m_force_zoom_reset_after_layer_transition = false;
    if (m_profile == nullptr) {
        m_profile = BlackflowRoutingInfo.strategy_for_mode(static_cast<int>(m_config->get_mode()));
    }
}

bool RoguelikeBlackflowRoutingTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    std::string task_name = details.get("details", "task", "");

    // trigger 任务名可以带 '-' 后缀变体（截断后匹配）
    if (const size_t pos = task_name.find('-'); pos != std::string::npos) {
        task_name = task_name.substr(0, pos);
    }

    return task_name == m_config->get_theme() + "@Roguelike@Routing";
}

// ============================================================================
// 地图识别
// ============================================================================
RoguelikeBlackflowMapAnalyzer::Result RoguelikeBlackflowRoutingTaskPlugin::recognize_map()
{
    // 1. 确保地图缩小：同一帧比较“＋”和“−”的置信率。
    //    游戏在最小缩放时仍会保留“−”按钮，因此不能以“−”消失作为退出条件。
    //    最小缩放时“−”模板会产生约 0.95 的背景误匹配，真实按钮通常 >= 0.99。
    constexpr double zoom_threshold = 0.99;
    constexpr int zoom_max_clicks = 8;
    constexpr int zoom_wait_ms = 1000;
    constexpr double layer_reset_threshold = 0.8;
    constexpr const char* zoom_in_template = "Blackflow@Roguelike@MapZoomIn.png";
    constexpr const char* zoom_out_template = "Blackflow@Roguelike@MapZoomOut.png";
    const Rect zoom_roi { 0, 540, 120, 100 };

    if (m_force_zoom_reset_after_layer_transition) {
        const cv::Mat image = ctrler()->get_image();
        if (!image.empty()) {
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
            if (zoom_in_score >= zoom_out_score && zoom_in && zoom_in_score >= layer_reset_threshold) {
                const Point click_point {
                    zoom_in->rect.x + zoom_in->rect.width / 2,
                    zoom_in->rect.y + zoom_in->rect.height / 2
                };
                Log.info(__FUNCTION__, "| layer transition reset: clicking zoom in at", click_point.x, click_point.y);
                ctrler()->click(click_point);
                m_force_zoom_reset_after_layer_transition = false;
                sleep(zoom_wait_ms);
            }
            else if (zoom_out && zoom_out_score >= layer_reset_threshold) {
                const Point click_point {
                    zoom_out->rect.x + zoom_out->rect.width / 2,
                    zoom_out->rect.y + zoom_out->rect.height / 2
                };
                Log.info(__FUNCTION__, "| layer transition reset: clicking zoom out at", click_point.x, click_point.y);
                ctrler()->click(click_point);
                m_force_zoom_reset_after_layer_transition = false;
                sleep(zoom_wait_ms);
            }
            else {
                Log.warn(
                    __FUNCTION__,
                    "| layer transition reset icon not found, in=",
                    zoom_in_score,
                    "out=",
                    zoom_out_score);
            }
        }
    }

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
    ProcessTask(*this, { "Blackflow@RoguelikeRouting-SwipeToCorner" }).run();
    sleep(300);

    // 3. 截图识别
    cv::Mat image = ctrler()->get_image();
    utils::save_debug_image(
        image,
        utils::path("debug") / "roguelikeMap",
        true,
        "drowningseekers recognition input",
        "input");
    RoguelikeBlackflowMapAnalyzer analyzer(image);
    return analyzer.analyze();
}

int RoguelikeBlackflowRoutingTaskPlugin::recognize_action_points(const char* task_name)
{
    cv::Mat image = ctrler()->get_image().clone();
    const auto task_ptr = Task.get<OcrTaskInfo>(task_name);
    if (!task_ptr) {
        Log.error("BlackflowRouting | action points OCR task missing", task_name);
        return -1;
    }

    const cv::Rect roi = make_rect<cv::Rect>(task_ptr->roi);

    struct Candidate
    {
        int value = -1;
        double score = 0.0;
        std::string text;
    };

    auto recognize = [&](const cv::Mat& input, const char* pass) -> std::optional<Candidate> {
        OCRer analyzer(input);
        analyzer.set_task_info(task_name);
        analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
        analyzer.set_use_char_model(true);
        if (!analyzer.analyze()) {
            Log.info("BlackflowRouting | action points OCR", task_name, pass, "no result");
            return std::nullopt;
        }

        std::optional<Candidate> best;
        for (const auto& result : analyzer.get_result()) {
            if (result.text.empty() ||
                !std::ranges::all_of(result.text, [](char ch) { return ch >= '0' && ch <= '9'; })) {
                continue;
            }
            int value = 0;
            if (!utils::chars_to_number(result.text, value) || value < 0 || value > 99) {
                continue;
            }
            if (!best || result.score > best->score) {
                best = Candidate { value, result.score, result.text };
            }
        }

        if (best) {
            Log.info(
                "BlackflowRouting | action points OCR",
                task_name,
                pass,
                best->text,
                "value",
                best->value,
                "score",
                best->score);
        }
        else {
            Log.info("BlackflowRouting | action points OCR", task_name, pass, "no numeric result");
        }
        return best;
    };

    // 先识别原图，保留细小数字的抗锯齿；再识别白色过滤图，抑制地图背景。
    const auto raw = recognize(image, "raw");

    cv::Mat gray;
    cv::cvtColor(image(roi), gray, cv::COLOR_BGR2GRAY);
    cv::Mat white_mask;
    constexpr int kActionPointWhiteThreshold = 180;
    cv::inRange(gray, kActionPointWhiteThreshold, 255, white_mask);
    cv::Mat filtered = image.clone();
    cv::Mat filtered_roi;
    cv::cvtColor(white_mask, filtered_roi, cv::COLOR_GRAY2BGR);
    filtered_roi.copyTo(filtered(roi));
    const auto filtered_result = recognize(filtered, "white-filtered");

    if (raw) {
        return raw->value;
    }
    if (filtered_result) {
        return filtered_result->value;
    }
    return -1;
}

// ============================================================================
// 加工品面板
// ============================================================================
void RoguelikeBlackflowRoutingTaskPlugin::ensure_gear_panel_closed()
{
    // 显示玩家按钮命中则点击（收起面板并聚焦玩家）；未命中经 Stop 立即返回。
    ProcessTask(*this, { "Blackflow@RoguelikeRouting-CloseGearPanel", "Stop" }).run();
}

void RoguelikeBlackflowRoutingTaskPlugin::open_gear_panel()
{
    ProcessTask(*this, { "Blackflow@RoguelikeRouting-OpenGearPanel" }).run();
}

std::vector<RoguelikeBlackflowRoutingTaskPlugin::GearCardHit>
    RoguelikeBlackflowRoutingTaskPlugin::ocr_gear_cards()
{
    std::vector<GearCardHit> cards;

    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("Blackflow@Roguelike@GearPanelOcr");
    if (!analyzer.analyze()) {
        return cards;
    }

    std::vector<std::string> known_names { kWalkName };
    for (const auto& gear : BlackflowRoutingInfo.gears()) {
        known_names.emplace_back(gear.name);
    }

    struct Badge
    {
        int cy = 0;
        int value = 0;
    };

    struct NameHit
    {
        std::string name;
        int cy = 0;
        bool inline_loaded = false; // "装载中"与名称被 OCR 合并进同一个框
    };

    std::vector<Badge> badges;
    std::vector<int> loaded_cys;
    std::vector<NameHit> names;

    for (const auto& res : analyzer.get_result()) {
        const int cy = res.rect.y + res.rect.height / 2;
        if (auto uses = parse_uses_badge(res.text)) {
            badges.push_back({ cy, *uses });
            continue;
        }
        const bool has_loaded_mark = res.text.find("装载中") != std::string::npos;
        bool matched_name = false;
        for (const auto& name : known_names) {
            if (res.text.find(name) != std::string::npos) {
                names.push_back({ name, cy, has_loaded_mark });
                matched_name = true;
                break;
            }
        }
        if (has_loaded_mark && !matched_name) {
            loaded_cys.push_back(cy);
        }
    }

    for (const auto& hit : names) {
        GearCardHit card;
        card.name = hit.name;
        card.name_cy = hit.cy;
        card.loaded = hit.inline_loaded;
        if (hit.name != kWalkName) {
            int best_dist = INT_MAX;
            for (const auto& badge : badges) {
                const int dist = hit.cy - badge.cy;
                if (dist >= kBadgeWindowMin && dist <= kBadgeWindowMax && dist < best_dist) {
                    best_dist = dist;
                    card.uses = badge.value;
                }
            }
        }
        if (!card.loaded) {
            for (const int lcy : loaded_cys) {
                const int dist = hit.cy - lcy;
                if (dist >= kLoadedWindowMin && dist <= kLoadedWindowMax) {
                    card.loaded = true;
                    break;
                }
            }
        }
        cards.emplace_back(std::move(card));
    }

    std::ranges::sort(cards, {}, &GearCardHit::name_cy);
    return cards;
}

RoguelikeBlackflowRoutingTaskPlugin::GearPanelInfo RoguelikeBlackflowRoutingTaskPlugin::read_gear_panel()
{
    LogTraceFunction;

    GearPanelInfo info;
    bool walk_seen = false;

    // 先把可能残留的滚动位置推回顶部，再从顶部向下逐页识别。
    for (int reset = 0; reset < kMaxPanelScrolls && !need_exit(); ++reset) {
        ProcessTask(*this, { "Blackflow@RoguelikeRouting-GearPanelSwipeToTop" }).run();
    }

    for (int screen = 0; screen <= kMaxPanelScrolls && !need_exit(); ++screen) {
        std::vector<GearCardHit> cards = ocr_gear_cards();
        if (cards.empty() && screen == 0) {
            sleep(500); // 面板展开动画未完成时重试一次
            cards = ocr_gear_cards();
        }

        bool any_new = false;
        for (const auto& card : cards) {
            if (card.loaded) {
                info.loaded_name = card.name;
            }
            if (card.name == kWalkName) {
                if (!walk_seen) {
                    walk_seen = true;
                    any_new = true;
                }
                continue;
            }
            const bool exists = std::ranges::any_of(info.uses_by_name, [&](const auto& pair) {
                return pair.first == card.name;
            });
            if (!exists) {
                // 徽章 OCR 失败时保守按剩余 1 次
                info.uses_by_name.emplace_back(card.name, card.uses > 0 ? card.uses : 1);
                any_new = true;
            }
        }

        if (!any_new || screen == kMaxPanelScrolls) {
            break;
        }
        ProcessTask(*this, { "Blackflow@RoguelikeRouting-GearPanelSwipeUp" }).run();
    }

    info.valid = walk_seen || !info.uses_by_name.empty();
    return info;
}

bool RoguelikeBlackflowRoutingTaskPlugin::select_gear_card(const std::string& name)
{
    LogTraceFunction;
    Log.info(__FUNCTION__, "| selecting gear card:", name);

    for (int attempt = 0; attempt < 2 && !need_exit(); ++attempt) {
        for (int screen = 0; screen <= kMaxPanelScrolls && !need_exit(); ++screen) {
            const auto cards = ocr_gear_cards();
            const auto it = std::ranges::find_if(cards, [&](const auto& card) { return card.name == name; });
            if (it != cards.end()) {
                for (int click_try = 0; click_try < 2 && !need_exit(); ++click_try) {
                    ctrler()->click(Point(kCardClickX, it->name_cy));
                    sleep(600);
                    const auto verify_cards = ocr_gear_cards();
                    const auto vit =
                        std::ranges::find_if(verify_cards, [&](const auto& card) { return card.name == name; });
                    if (vit != verify_cards.end() && vit->loaded) {
                        return true;
                    }
                    Log.warn(__FUNCTION__, "| clicked gear card but not loaded:", name);
                }
                return false; // 点击两次仍未装载，异常交给上层重试
            }
            if (screen < kMaxPanelScrolls) {
                ProcessTask(*this, { "Blackflow@RoguelikeRouting-GearPanelSwipeUp" }).run();
            }
        }
        // 收起重开以复位滚动位置
        ensure_gear_panel_closed();
        open_gear_panel();
    }
    return false;
}

// ============================================================================
// 规划输入翻译
// ============================================================================
drowning_seekers::PlannerMap RoguelikeBlackflowRoutingTaskPlugin::build_planner_map(
    const RoguelikeBlackflowMapAnalyzer::Result& result) const
{
    const auto& cfg = BlackflowRoutingInfo;

    drowning_seekers::PlannerMap pm;
    pm.cols = result.cols;
    pm.rows = result.rows;
    const int n = pm.cols * pm.rows;
    pm.cells.resize(n);
    pm.adj.resize(n);
    pm.player = result.player.second * pm.cols + result.player.first;

    std::vector<int> teleport_cells;
    for (const auto& c : result.cells) {
        const int idx = c.row * pm.cols + c.col;
        auto& pc = pm.cells[idx];
        pc.exists = true;
        pc.visited = c.visited;
        if (c.kind != RoguelikeBlackflowMapAnalyzer::CellKind::Object) {
            continue;
        }
        if (auto wit = m_profile->node_weights.find(c.type); wit != m_profile->node_weights.end()) {
            pc.weight = wit->second;
        }
        pc.ap_gain = cfg.node_ap_gain(c.type);
        pc.is_endpoint = cfg.is_endpoint(c.type);
        pc.is_combat = cfg.is_combat(c.type);
        pc.is_trader = cfg.is_trader(c.type);
        if (cfg.node_teleport_paired(c.type)) {
            teleport_cells.push_back(idx);
        }
    }

    for (const auto& [a, b] : result.edges) {
        const int ia = a.second * pm.cols + a.first;
        const int ib = b.second * pm.cols + b.first;
        if (ia < 0 || ia >= n || ib < 0 || ib >= n) {
            continue;
        }
        pm.adj[ia].push_back(ib);
        pm.adj[ib].push_back(ia);
    }
    for (auto& neighbors : pm.adj) {
        std::ranges::sort(neighbors);
        neighbors.erase(std::ranges::unique(neighbors).begin(), neighbors.end());
    }

    // 曲折密道：恰好成对时互设传送；数量异常则不建传送模型
    if (teleport_cells.size() == 2) {
        pm.cells[teleport_cells[0]].teleport_twin = teleport_cells[1];
        pm.cells[teleport_cells[1]].teleport_twin = teleport_cells[0];
    }
    else if (!teleport_cells.empty()) {
        Log.warn(
            "BlackflowRouting | winding passages not in pair:",
            teleport_cells.size(),
            "- teleport modeling disabled");
    }

    return pm;
}

std::vector<drowning_seekers::PlannerGear> RoguelikeBlackflowRoutingTaskPlugin::build_planner_gears(
    const GearPanelInfo& panel,
    std::vector<std::string>& gear_names) const
{
    const auto& cfg = BlackflowRoutingInfo;

    std::vector<drowning_seekers::PlannerGear> gears;
    gear_names.clear();
    for (const auto& [name, uses] : panel.uses_by_name) {
        const BlackflowGearInfo* info = cfg.gear_by_name(name);
        if (info == nullptr) {
            Log.warn("BlackflowRouting | unknown gear from panel OCR:", name);
            continue;
        }
        drowning_seekers::PlannerGear gear;
        gear.range = info->range;
        gear.distance = info->distance;
        gear.uses = std::clamp(uses, 0, info->max_uses);
        gear.ap_cost = info->ap_cost;
        gear.ap_gain = info->ap_gain;
        gear.use_cost = info->carryover ? m_profile->gear_use_cost : m_profile->non_carryover_use_cost;
        if (auto rit = m_profile->gear_use_reward.find(name); rit != m_profile->gear_use_reward.end()) {
            gear.use_reward = rit->second;
        }
        gear.carryover = info->carryover;
        gear.controllable = info->controllable;
        gears.emplace_back(gear);
        gear_names.emplace_back(name);
    }
    return gears;
}

// ============================================================================
// 日志与收尾
// ============================================================================
void RoguelikeBlackflowRoutingTaskPlugin::dump_recognition(
    const RoguelikeBlackflowMapAnalyzer::Result& result,
    int action_points) const
{
    Log.info(
        "BlackflowRouting | === recognition dump ===",
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
                else if (c.kind == RoguelikeBlackflowMapAnalyzer::CellKind::Object) {
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
            c.kind == RoguelikeBlackflowMapAnalyzer::CellKind::Object ? type2name(c.type) : std::string("road"),
            c.visited ? "[visited]" : "",
            c.ocr_text.empty() ? "" : ("ocr=" + c.ocr_text));
    }
    for (const auto& [a, b] : result.edges) {
        Log.info("  edge (", a.first, ",", a.second, ")-(", b.first, ",", b.second, ")");
    }
}

void RoguelikeBlackflowRoutingTaskPlugin::act_abandon(const std::string& reason)
{
    Log.info("BlackflowRouting | abandon:", reason);
    callback(
        AsstMsg::SubTaskExtraInfo,
        [&] {
            auto info = basic_info_with_what("BlackflowRoutingDecision");
            info["details"]["action"] = "abandon";
            info["details"]["reason"] = reason;
            return info;
        }());
    const std::string exit_task = reason == "boil_water_success_dreadful_foe"
        ? "Blackflow@RoguelikeRoutingAction-ExitThenAbandon_ToHardest"
        : "Blackflow@RoguelikeRoutingAction-ExitThenAbandon";
    Task.set_task_base("RoguelikeRoutingAction", exit_task);
}

void RoguelikeBlackflowRoutingTaskPlugin::act_retry(const std::string& reason)
{
    ++m_consecutive_failures;
    Log.warn("BlackflowRouting | retry:", reason, "| consecutive", m_consecutive_failures);
    if (m_consecutive_failures >= kMaxConsecutiveFailures) {
        act_abandon("consecutive_failures:" + reason);
        return;
    }
    callback(
        AsstMsg::SubTaskExtraInfo,
        [&] {
            auto info = basic_info_with_what("BlackflowRoutingDecision");
            info["details"]["action"] = "retry";
            info["details"]["reason"] = reason;
            return info;
        }());
    Task.set_task_base("RoguelikeRoutingAction", "Blackflow@RoguelikeRoutingAction-Retry");
}

// ============================================================================
// 主流程
// ============================================================================
bool RoguelikeBlackflowRoutingTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_profile == nullptr) {
        return false;
    }

    // 1. 收起可能残留的加工品面板，识别地图与行动力（面板关闭态）
    ensure_gear_panel_closed();
    const RoguelikeBlackflowMapAnalyzer::Result result = recognize_map();
    if (!result.valid) {
        Log.error("BlackflowRouting | map recognition failed");
        act_retry("recognition_failed");
        return true;
    }

    // 烧水只需验证是否已经进入第三层：险路恶敌只会出现在第三层和第五层。
    // 识别到该节点即可判定成功，不再打开加工品面板或继续规划路线。
    if (m_profile->name == "boilWater") {
        const auto dreadful_foe = std::ranges::find_if(result.cells, [](const auto& cell) {
            return cell.type == RoguelikeNodeType::DreadfulFoe;
        });
        if (dreadful_foe != result.cells.end()) {
            Log.info(
                "BlackflowRouting | boilWater success: detected DreadfulFoe at",
                dreadful_foe->col,
                dreadful_foe->row);
            act_abandon("boil_water_success_dreadful_foe");
            return true;
        }
    }

    const int map_action_points = recognize_action_points("Blackflow@Roguelike@ActionPointsRecognition");

    // 2. 打开加工品面板，再识别一次行动力与加工品（面板态作为备用）
    open_gear_panel();
    const int panel_action_points =
        recognize_action_points("Blackflow@Roguelike@ActionPointsRecognitionGearPanel");
    const int action_points = panel_action_points >= 0 ? panel_action_points : map_action_points;
    Log.info(
        "BlackflowRouting | action points OCR map",
        map_action_points,
        "panel",
        panel_action_points,
        "selected",
        action_points,
        "source",
        panel_action_points >= 0 ? "panel" : "map");
    GearPanelInfo panel = read_gear_panel();
    if (!panel.valid) {
        ensure_gear_panel_closed();
        open_gear_panel();
        panel = read_gear_panel();
    }

    dump_recognition(result, action_points);
    for (const auto& [name, uses] : panel.uses_by_name) {
        Log.info("  gear", name, "x", uses, name == panel.loaded_name ? "[loaded]" : "");
    }
    Log.info("  loaded:", panel.loaded_name.empty() ? "<unknown>" : panel.loaded_name);

    if (action_points < 0) {
        ensure_gear_panel_closed();
        act_retry("action_points_unknown");
        return true;
    }
    if (!panel.valid) {
        // 面板读不到时无法确认当前装载的移动方式，贸然点击节点可能误耗加工品
        ensure_gear_panel_closed();
        act_retry("gear_panel_unreadable");
        return true;
    }

    // 3. 翻译 + 束搜索规划
    const drowning_seekers::PlannerMap pmap = build_planner_map(result);
    std::vector<std::string> gear_names;
    const std::vector<drowning_seekers::PlannerGear> pgears = build_planner_gears(panel, gear_names);

    drowning_seekers::PlannerParams params;
    params.action_points = action_points;
    params.endpoint_required = m_profile->endpoint_required;
    params.shortest_endpoint = m_profile->shortest_endpoint;
    params.avoid_combat_first = m_profile->avoid_combat_first;
    params.best_effort = m_profile->best_effort_when_unreachable;
    params.leftover_ap_weight = m_profile->leftover_ap_weight;
    const drowning_seekers::PlannerResult planned = drowning_seekers::plan(pmap, pgears, params);

    const std::string route_str = route_to_string(planned, pmap);
    Log.info(
        "BlackflowRouting | strategy",
        m_profile->name,
        "| planned score",
        planned.score,
        "| reaches_endpoint",
        planned.reaches_endpoint,
        "| route:",
        route_str.empty() ? "<none>" : route_str);

    // 回传识别与规划摘要给 WPF
    callback(
        AsstMsg::SubTaskExtraInfo,
        [&] {
            auto info = basic_info_with_what("BlackflowMapRecognition");
            info["details"]["cols"] = result.cols;
            info["details"]["rows"] = result.rows;
            info["details"]["nodes"] = static_cast<int>(result.cells.size());
            info["details"]["edges"] = static_cast<int>(result.edges.size());
            info["details"]["player_col"] = result.player.first;
            info["details"]["player_row"] = result.player.second;
            info["details"]["action_points"] = action_points;
            info["details"]["map_ascii"] = format_map_ascii(result);
            info["details"]["strategy"] = m_profile->name;
            auto gears_json = json::array();
            for (const auto& [name, uses] : panel.uses_by_name) {
                gears_json.emplace_back(json::object { { "name", name }, { "uses", uses } });
            }
            info["details"]["gears"] = std::move(gears_json);
            info["details"]["loaded"] = panel.loaded_name;
            info["details"]["planned_route"] = route_str;
            info["details"]["planned_score"] = planned.score;
            info["details"]["reaches_endpoint"] = planned.reaches_endpoint;
            return info;
        }());

    if (m_dry_run) {
        // 仅识别与规划，安全退出以便人工核对（M1）。
        ensure_gear_panel_closed();
        act_abandon("dry_run");
        return true;
    }

    // 4. 放弃裁决
    if (m_profile->abandon_when_no_positive) {
        const bool has_free_move = std::ranges::any_of(pgears, [](const auto& gear) {
            return gear.controllable && gear.uses > 0 && gear.ap_cost == 0;
        });
        if (action_points <= m_abandon_ap && !has_free_move) {
            ensure_gear_panel_closed();
            act_abandon("action_points_too_low");
            return true;
        }
    }
    if (!planned.has_route) {
        ensure_gear_panel_closed();
        act_abandon(m_profile->endpoint_required ? "endpoint_unreachable" : "no_route");
        return true;
    }
    if (m_profile->abandon_when_no_positive && planned.score <= 0.0) {
        ensure_gear_panel_closed();
        act_abandon("no_positive_route");
        return true;
    }

    // 5. 执行首个动作：按需切换移动方式
    const drowning_seekers::PlannerAction& first = planned.actions.front();
    const std::string desired_mode = first.is_walk ? kWalkName : gear_names.at(first.gear_index);
    if (desired_mode != panel.loaded_name) {
        if (!select_gear_card(desired_mode)) {
            ensure_gear_panel_closed();
            act_retry("select_gear_failed:" + desired_mode);
            return true;
        }
    }
    ensure_gear_panel_closed();

    // 6. 面板关闭后直接使用已识别的地图和目标节点，不再滑动或重复识别
    const int target_col = first.target % pmap.cols;
    const int target_row = first.target / pmap.cols;
    const RoguelikeBlackflowMapAnalyzer::Cell* target_cell = nullptr;
    for (const auto& c : result.cells) {
        if (c.col == target_col && c.row == target_row) {
            target_cell = &c;
            break;
        }
    }
    if (target_cell == nullptr) {
        Log.error("BlackflowRouting | planned target is missing from recognized map");
        act_retry("target_missing");
        return true;
    }

    m_consecutive_failures = 0;

    const int final_target = planned.actions.back().target;
    m_force_zoom_reset_after_layer_transition = target_cell->type == RoguelikeNodeType::DreadfulFoe ||
        target_cell->type == RoguelikeNodeType::PathEnd || target_cell->type == RoguelikeNodeType::PathLane;
    if (m_force_zoom_reset_after_layer_transition) {
        Log.info(
            "BlackflowRouting | next map will reset zoom after layer node",
            type2name(target_cell->type));
    }
    Log.info(
        "BlackflowRouting | move via",
        desired_mode,
        "to (",
        target_col,
        ",",
        target_row,
        ") type",
        type2name(target_cell->type),
        "| route target (",
        final_target % pmap.cols,
        ",",
        final_target / pmap.cols,
        ")");

    callback(
        AsstMsg::SubTaskExtraInfo,
        [&] {
            auto info = basic_info_with_what("BlackflowRoutingDecision");
            info["details"]["action"] = "move";
            info["details"]["reason"] = "planned";
            info["details"]["move_mode"] = desired_mode;
            info["details"]["next_col"] = target_col;
            info["details"]["next_row"] = target_row;
            info["details"]["next_type"] = type2name(target_cell->type);
            info["details"]["target_col"] = final_target % pmap.cols;
            info["details"]["target_row"] = final_target / pmap.cols;
            return info;
        }());

    // 点击目标格中心，继续使用当前工作区的按节点类型分流；道路、羽瞰点和曲折
    // 密道点击后不进入节点页面，直接回到地图循环。
    ctrler()->click(Point(target_cell->center.x, target_cell->center.y));
    sleep(300);

    const bool moves_without_stage =
        target_cell->kind == RoguelikeBlackflowMapAnalyzer::CellKind::Road ||
        target_cell->type == RoguelikeNodeType::VantagePoint || target_cell->type == RoguelikeNodeType::WindingPassage;
    if (moves_without_stage) {
        Task.set_task_base("RoguelikeRoutingAction", "Blackflow@RoguelikeRoutingAction-DirectReturn");
        ProcessTask(*this, { "Blackflow@Roguelike@StageUnknownOrEmptyEnterDirectReturn" }).run();
        return true;
    }

    if (const char* route_action = known_node_route_action(target_cell->type); route_action != nullptr) {
        Log.info("BlackflowRouting | typed node entry:", type2name(target_cell->type), "->", route_action);
        Task.set_task_base("RoguelikeRoutingAction", route_action);
        return true;
    }

    Task.set_task_base("RoguelikeRoutingAction", "Blackflow@RoguelikeRoutingAction-EnterNode");
    return true;
}
