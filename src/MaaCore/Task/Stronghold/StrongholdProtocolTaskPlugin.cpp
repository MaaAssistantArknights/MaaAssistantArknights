#include "StrongholdProtocolTaskPlugin.h"

#include "StrongholdTierMatcher.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/StrongholdProtocol/StrongholdProtocolConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Controller/ControllerAPI.h"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/PipelineAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

#include <MaaUtils/NoWarningCV.hpp>

#include <algorithm>
#include <chrono>
#include <tuple>

namespace asst
{
// 缺少局部区域时返回空框，调用方跳过该次识别。
static Rect task_roi(const std::string& task_name)
{
    if (auto task = Task.get(task_name); task && task->roi.width > 0 && task->roi.height > 0) {
        return task->roi;
    }
    LogError << __FUNCTION__ << "roi task missing or empty:" << task_name;
    return {};
}

static bool has_facing_cancel(const cv::Mat& image, const Point& tile)
{
    const auto task = Task.get<OcrTaskInfo>("StrongholdProtocol@FacingCancelOcr");
    if (!task || task->roi.empty()) {
        return false;
    }
    auto roi = task->roi;
    roi.x += tile.x;
    roi.y += tile.y;
    OCRer facing(image);
    facing.set_task_info(task);
    facing.set_roi(roi);
    return facing.analyze().has_value();
}

bool StrongholdProtocolTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    return task == "StrongholdProtocol@Farm@Begin";
}

StrongholdProtocolTaskPlugin& StrongholdProtocolTaskPlugin::set_target(int target) noexcept
{
    m_target = target;
    return *this;
}

StrongholdProtocolTaskPlugin& StrongholdProtocolTaskPlugin::set_runs(int runs) noexcept
{
    m_runs = runs;
    return *this;
}

StrongholdProtocolTaskPlugin& StrongholdProtocolTaskPlugin::set_attempts(int attempts) noexcept
{
    m_attempts = attempts;
    return *this;
}

bool StrongholdProtocolTaskPlugin::_run()
{
    LogTraceFunction;

    BoardState board;
    m_bench_shop_unconfirmed = false;
    m_last_tile.reset();
    // 选定策略、推进到新回合或结算时刷新进度，战斗和过场保留原来的计时。
    constexpr auto stall_timeout = std::chrono::minutes(10);
    // 连续看不到活动界面时提前停止。
    constexpr auto leave_timeout = std::chrono::seconds(30);
    auto last_progress = std::chrono::steady_clock::now();
    auto last_in_garrison = last_progress;
    while (!need_exit() && m_cleared < m_runs && (m_attempts <= 0 || m_attempted < m_attempts)) {
        if (std::chrono::steady_clock::now() - last_progress > stall_timeout) {
            save_img(utils::path("debug") / utils::path("stronghold"), false);
            LogError << __FUNCTION__ << "no progress for" << stall_timeout.count() << "minutes, task stopped";
            return false;
        }

        bool progressed = false;
        const auto image = ctrler()->get_image();
        if (m_last_tile && has_facing_cancel(image, *m_last_tile)) {
            if (set_facing(*m_last_tile)) {
                m_last_tile.reset();
            }
            last_in_garrison = std::chrono::steady_clock::now();
            continue;
        }
        Scene scene = detect_scene(image);
        if (scene != Scene::Unknown) {
            last_in_garrison = std::chrono::steady_clock::now();
        }
        if (scene != Scene::Settlement && scene != Scene::Transition && scene != Scene::Unknown) {
            m_settlement_counted = false; // 已离开结算页进入下一局流程，下次结算重新计数
        }
        if (scene == Scene::StrategySelect) {
            // 确认策略时已识别出下一页，同一轮直接处理该页，操作本身仍使用新截图。
            const auto result = pick_strategy(scene);
            if (need_exit() || result == StrategyResult::IncompleteScan) {
                return false;
            }
            if (result == StrategyResult::AllCompleted) {
                auto info = basic_info_with_what("StrongholdStrategiesCompleted");
                info["details"]["target"] = m_target;
                callback(AsstMsg::SubTaskExtraInfo, info);
                return true;
            }
            progressed = true;
        }
        switch (scene) {
        case Scene::Navigate:
            // 活动主界面：走 @Nav 链导航到策略选择页（链停在策略页，不代选策略）
            ProcessTask(*this, { "StrongholdProtocol@Nav" }).run();
            break;
        case Scene::StartSimulation:
            ProcessTask(*this, { "StrongholdProtocol@Nav@StartSimulation" }).run();
            break;
        case Scene::BountySelect:
            ProcessTask(*this, { "StrongholdProtocol@BountyTitle" }).set_retry_times(1).run();
            break;
        case Scene::Reconnect:
            ProcessTask(*this, { "StrongholdProtocol@ReconnectAvailable" }).set_retry_times(0).run();
            break;
        case Scene::BattleTimeout:
            ProcessTask(*this, { "StrongholdProtocol@BattleTimeout" }).set_retry_times(0).run();
            break;
        case Scene::ConfirmInfo:
            click_template("StrongholdProtocol@Nav@ReadyScreen");
            break;
        case Scene::CustomSquad:
            // 自选编队页：pick_squad 按权重选人后自行点确认
            pick_squad(board);
            break;
        case Scene::RoundPrepare: {
            const int round_before = board.round_done;
            if (!prep_phase(board)) {
                LogWarn << __FUNCTION__ << "preparation stopped with unconfirmed shop layout";
                return false;
            }
            progressed |= board.round_done != round_before;
            break;
        }
        case Scene::ConfirmPopup:
            handle_popups();
            break;
        case Scene::Settlement:
            progressed = !m_settlement_counted;
            if (!handle_settlement(board)) {
                return false;
            }
            break;
        case Scene::Battle:
            sleep(800);
            continue;
        case Scene::Transition:
            sleep(500);
            continue;
        case Scene::Unknown:
            if (std::chrono::steady_clock::now() - last_in_garrison > leave_timeout) {
                save_img(utils::path("debug") / utils::path("stronghold"), false);
                LogError << __FUNCTION__ << "stronghold protocol screen not seen for" << leave_timeout.count()
                         << "seconds, task stopped";
                return false;
            }
            sleep(500);
            continue;
        case Scene::StrategySelect:
            break;
        }
        if (progressed) {
            last_progress = std::chrono::steady_clock::now();
        }
        sleep(400);
    }
    return true;
}

StrongholdProtocolTaskPlugin::Scene StrongholdProtocolTaskPlugin::detect_scene(const cv::Mat& image)
{
    PipelineAnalyzer analyzer(image);
    const auto has = [&](const std::string& name) {
        analyzer.set_tasks({ "StrongholdProtocol@" + name });
        return analyzer.analyze().has_value();
    };
    if (has("ReturnHome")) {
        return Scene::Settlement;
    }
    // 悬赏和确认弹窗可能露出背景页的按钮，须先识别弹窗。
    if (has("BountyTitle")) {
        return has("BountyMiddleCard") ? Scene::BountySelect : Scene::Unknown;
    }
    if (has("ConfirmReadyOcr")) {
        return Scene::ConfirmPopup;
    }
    if (has("BattleTimeout")) {
        return has("BattleTimeoutConfirm") ? Scene::BattleTimeout : Scene::Unknown;
    }
    const bool squad_button = has("CustomOpersConfirm");
    const bool info_button = has("Nav@ReadyScreen");
    if (squad_button || info_button) {
        if (squad_button && has("CustomSquadTitle")) {
            return Scene::CustomSquad;
        }
        return info_button && has("ConfirmInfo") ? Scene::ConfirmInfo : Scene::Unknown;
    }
    if (has("SPRConfirmSelect") || has("SPAConfirmSelect") || has("Goliath")) {
        return has("StrategyTitle") ? Scene::StrategySelect : Scene::Unknown;
    }
    if (has("ReadyButton")) {
        return Scene::RoundPrepare;
    }
    if (has("ReconnectAvailable")) {
        return has("Reconnect") ? Scene::Reconnect : Scene::Unknown;
    }
    if (has("Nav@EnterQuickly")) {
        return Scene::Navigate;
    }
    if (has("Nav@StartSimulation")) {
        return Scene::StartSimulation;
    }
    if (has("SimulationHud")) {
        return has("BattleRound") || has("BattleInspect") || has("BattleDetailsOpen") ? Scene::Battle
                                                                                      : Scene::Transition;
    }
    return has("SimulationLoading") ? Scene::Transition : Scene::Unknown;
}

bool StrongholdProtocolTaskPlugin::click_template(const std::string& task_name)
{
    Matcher matcher(ctrler()->get_image());
    matcher.set_task_info(task_name);
    if (auto result = matcher.analyze()) {
        const Rect& rect = result->rect;
        return ctrler()->click(rect);
    }
    return false;
}

// ---------------- 干员判定（权重表） ----------------

static int opening_front_priority(const std::string& name)
{
    switch (StrongholdProtocol.get_role(name)) {
    case battle::Role::Tank:
        return 3;
    case battle::Role::Warrior:
        return 2;
    case battle::Role::Pioneer:
        return 1;
    case battle::Role::Medic:
    case battle::Role::Unknown:
    case battle::Role::Drone:
        return -1;
    default:
        return 0;
    }
}

bool StrongholdProtocolTaskPlugin::is_medic(const std::string& name)
{
    return StrongholdProtocol.get_role(name) == battle::Role::Medic;
}

std::optional<int> StrongholdProtocolTaskPlugin::measure_grid_shift(const cv::Mat& before, const cv::Mat& after)
{
    // 用滚动前的一条横带（Roi@StrategyGridStrip）在滚动后的网格可见区（Roi@StrategyGrid）里找位置，
    // 差值即滚动位移（向上为正）
    const cv::Rect grid_cv = make_rect<cv::Rect>(task_roi("StrongholdProtocol@Roi@StrategyGrid"));
    const cv::Rect strip_cv = make_rect<cv::Rect>(task_roi("StrongholdProtocol@Roi@StrategyGridStrip"));
    if (strip_cv.empty() || grid_cv.empty() || (strip_cv & cv::Rect(0, 0, before.cols, before.rows)) != strip_cv ||
        (grid_cv & cv::Rect(0, 0, after.cols, after.rows)) != grid_cv || grid_cv.height < strip_cv.height ||
        grid_cv.width < strip_cv.width) {
        return std::nullopt;
    }
    const cv::Mat strip = before(strip_cv);
    const cv::Mat area = after(grid_cv);
    cv::Mat result;
    cv::matchTemplate(area, strip, result, cv::TM_CCOEFF_NORMED);
    double max_val = 0;
    cv::Point max_loc;
    cv::minMaxLoc(result, nullptr, &max_val, nullptr, &max_loc);
    if (max_val < 0.6) {
        return std::nullopt;
    }
    return strip_cv.y - (grid_cv.y + max_loc.y);
}

// ---------------- 局外：选策略 / 自选编队 ----------------

static std::optional<int> read_strategy_passed(const cv::Mat& panel)
{
    OCRer count(panel);
    count.set_task_info("StrongholdProtocol@StrategyPassed");
    const auto rows = count.analyze();
    if (!rows) {
        return 0;
    }

    // 标记和数字可能分框，文字顺序随客户端而异。区域中只能有一个标记和一个次数。
    bool marked = false;
    std::optional<int> passed;
    for (const auto& row : *rows) {
        std::string_view digits = row.text;
        if (digits.starts_with('#')) {
            if (marked) {
                return std::nullopt;
            }
            marked = true;
            digits.remove_prefix(1);
        }
        if (digits.empty()) {
            continue;
        }
        int value = 0;
        if (passed || digits.size() > 2 || !utils::chars_to_number<int, true>(digits, value)) {
            return std::nullopt;
        }
        passed = value;
    }
    return marked ? passed : std::nullopt;
}

StrongholdProtocolTaskPlugin::StrategyResult StrongholdProtocolTaskPlugin::pick_strategy(Scene& next_scene)
{
    LogTraceFunction;

    const auto& layout = StrongholdProtocol.get_strategy_grid();
    const auto& bar = layout.scrollbar;
    const auto& frame = layout.frame;
    const auto& view = frame.viewport;
    const auto [track_top, track_bottom] = bar.track_y;
    auto scrollbar = [&](const cv::Mat& image) -> std::optional<cv::Rect> {
        // 打开详情后网格和滚动条一起左移。窄条内只有滑块会形成两侧留空的灰白色长条。
        const auto [pad_x, pad_y] = bar.scan_padding;
        for (const int x : bar.columns) {
            const cv::Rect strip(x - pad_x, track_top - pad_y, 2 * pad_x + 1, track_bottom - track_top + 2 * pad_y + 1);
            if ((strip & cv::Rect(0, 0, image.cols, image.rows)) != strip) {
                continue;
            }
            cv::Mat hsv, mask;
            cv::cvtColor(image(strip), hsv, cv::COLOR_BGR2HSV);
            cv::inRange(hsv, cv::Scalar(0, 0, bar.min_value), cv::Scalar(180, bar.max_saturation, 255), mask);
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (const auto& contour : contours) {
                const auto rect = cv::boundingRect(contour);
                if (rect.height >= bar.min_height && rect.width >= bar.thumb_width[0] &&
                    rect.width <= bar.thumb_width[1] && std::abs(rect.x + rect.width / 2 - pad_x) <= 1 &&
                    cv::contourArea(contour) > rect.area() * bar.min_fill) {
                    return rect + strip.tl();
                }
            }
        }
        return std::nullopt;
    };

    if (!ProcessTask(*this, { "StrongholdProtocol@StrategyGridProbe" }).run()) {
        return StrategyResult::IncompleteScan;
    }

    auto detect_card_frames = [&](const cv::Mat& img, const cv::Rect& thumb, bool& verified) {
        // 已知网格位置时逐列核对卡框，暗色立绘和无角标策略也有左边框。
        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        const auto band = gray.rowRange(view.y, view.y + view.height);
        const int view_left = thumb.x + thumb.width / 2 + view.x;
        const auto full_frame = [&](int height) {
            return height >= frame.height[0] && height <= frame.height[1];
        };
        std::vector<Point> cells;
        for (int offset = 0; offset < view.width; offset += frame.column_step) {
            const int left = view_left + offset;
            const auto column_mean = [&](int x, int width = 4) {
                cv::Mat mean;
                cv::reduce(band.colRange(x, x + width), mean, 1, cv::REDUCE_AVG, CV_32F);
                return mean;
            };
            const auto find_borders = [](cv::Mat mask) {
                cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, { 1, 5 }));
                std::vector<std::vector<cv::Point>> contours;
                cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
                std::vector<cv::Rect> borders;
                for (const auto& contour : contours) {
                    borders.emplace_back(cv::boundingRect(contour));
                }
                return borders;
            };
            const auto edge = column_mean(left);
            auto borders = find_borders(edge - column_mean(left - 4, 3) > 10);
            // 暗卡的边框和外部背景可能同亮，另外检查左右边框与各自内侧的相对亮度差。
            const auto edge_contrast = [](const cv::Mat& frame, const cv::Mat& inner) {
                cv::Mat contrast;
                cv::compare(cv::abs(frame - inner), (frame + inner) * 0.10, contrast, cv::CMP_GT);
                return contrast;
            };
            const auto inner_edges = find_borders(
                edge_contrast(edge, column_mean(left + frame.inner_columns[0])) &
                edge_contrast(column_mean(left + frame.right_edge), column_mean(left + frame.inner_columns[1])));
            for (const auto& rect : inner_edges) {
                if (rect.y > 0 && rect.br().y < band.rows && full_frame(rect.height) &&
                    std::ranges::none_of(
                        borders,
                        [&](const cv::Rect& found) {
                            return full_frame(found.height) && std::abs(found.br().y - rect.br().y) < 35;
                        })) {
                    borders.emplace_back(rect);
                }
            }
            for (const auto& rect : borders) {
                if (rect.y == 0 || rect.br().y == band.rows) {
                    // 相邻页有重叠，裁断的卡留给显示全卡的一页。首尾再有裁断则无法核全。
                    if (rect.height >= frame.min_partial_height &&
                        ((rect.y == 0 && thumb.y <= track_top + 2) ||
                         (rect.br().y == band.rows && thumb.br().y >= track_bottom - 2))) {
                        verified = false;
                    }
                    continue;
                }
                if (full_frame(rect.height)) {
                    cells.emplace_back(
                        left + frame.center_from_bottom_left[0],
                        view.y + rect.br().y + frame.center_from_bottom_left[1]);
                }
                else if (rect.height >= frame.min_partial_height) {
                    verified = false;
                }
            }
        }
        // 分组末行可以留空。若已识别卡的左邻缺失，无法确认整行都已核对。
        const int first_col = view_left + frame.center_from_bottom_left[0];
        for (const auto& cell : cells) {
            if (cell.x > first_col && std::ranges::none_of(cells, [&](const Point& other) {
                    return other.x == cell.x - frame.column_step && std::abs(other.y - cell.y) < 35;
                })) {
                verified = false;
                break;
            }
        }
        return cells;
    };

    // 按立绘和角标定位可选卡，即使卡框或滑块识别不全也能继续选策略。
    auto detect_grid_cells = [&](const cv::Mat& img, std::vector<Rect>& completed) {
        const cv::Rect grid_cv = make_rect<cv::Rect>(task_roi("StrongholdProtocol@Roi@StrategyGrid"));
        if (grid_cv.empty() || (grid_cv & cv::Rect(0, 0, img.cols, img.rows)) != grid_cv) {
            return std::vector<Point> {};
        }
        cv::Mat hsv;
        cv::cvtColor(img(grid_cv), hsv, cv::COLOR_BGR2HSV);

        struct Ev
        {
            double x, y;
        };
        std::vector<Ev> evidence;

        // 立绘亮像素块：闭运算连接素描纹理线条，垂直长核腐蚀断开跨卡的水平连接
        cv::Mat lit;
        cv::inRange(
            hsv,
            cv::Scalar(0, 0, layout.portrait_min_value),
            cv::Scalar(180, layout.portrait_max_saturation, 255),
            lit);
        cv::morphologyEx(lit, lit, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, { 7, 7 }));
        cv::erode(lit, lit, cv::getStructuringElement(cv::MORPH_RECT, { 1, 13 }));
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(lit, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        for (const auto& c : contours) {
            const auto rc = cv::boundingRect(c);
            // 高度下限排分区标题文字条，宽高上限排跨卡连通与右侧详情面板立绘
            if (rc.width * rc.height < 250 || rc.height < 50 || rc.height > 200 || rc.width > 260) {
                continue;
            }
            evidence.emplace_back(grid_cv.x + rc.x + rc.width / 2.0, grid_cv.y + rc.y + rc.height / 2.0);
        }

        const auto match_markers = [&](const char* name, bool meets_target) {
            const auto task = Task.get<MatchTaskInfo>(name);
            if (!task || task->roi.empty() || task->rect_move.empty()) {
                return;
            }
            MultiMatcher matcher(img);
            matcher.set_task_info(task);
            if (const auto hits = matcher.analyze()) {
                for (const auto& hit : *hits) {
                    const Rect cell = hit.rect.move(task->rect_move);
                    evidence.emplace_back(cell.x + cell.width / 2.0, cell.y + cell.height / 2.0);
                    if (meets_target) {
                        completed.emplace_back(cell);
                    }
                }
            }
        };
        match_markers("StrongholdProtocol@StrategyNewMarker", false);
        match_markers("StrongholdProtocol@StrategyGoldMarker", m_target <= 3);
        match_markers("StrongholdProtocol@StrategyColorMarker", true);
        match_markers("StrongholdProtocol@StrategyDisabledMarker", false);

        // 列心吸附过滤杂散（顶栏图标/左栏暗红横幅/详情面板亮块的证据落不到列心附近），再按卡去重
        std::ranges::sort(evidence, [](const Ev& a, const Ev& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
        std::vector<Ev> dedup;
        for (const auto& e : evidence) {
            double best = 1e9;
            for (int c : layout.columns) {
                best = std::min(best, std::abs(e.x - c));
            }
            if (best > 45) {
                continue;
            }
            bool dup = false;
            for (const auto& q : dedup) {
                if ((e.x - q.x) * (e.x - q.x) + (e.y - q.y) * (e.y - q.y) < 40 * 40) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                dedup.emplace_back(e);
            }
        }

        // 行聚类（y 容差 35），行内证据吸附列心（单证据行要求更贴列心，防面板立绘孤立块）、
        // 行 y 取中位，输出卡心
        std::vector<Point> cells;
        std::vector<std::vector<Ev>> rows;
        for (const auto& e : dedup) {
            if (!rows.empty()) {
                double mean = 0;
                for (const auto& q : rows.back()) {
                    mean += q.y;
                }
                mean /= rows.back().size();
                if (std::abs(e.y - mean) <= 35) {
                    rows.back().emplace_back(e);
                    continue;
                }
            }
            rows.emplace_back(1, e);
        }
        for (auto& row : rows) {
            std::vector<int> cols_hit;
            for (const auto& e : row) {
                int best_col = layout.columns[0];
                for (int c : layout.columns) {
                    if (std::abs(e.x - c) < std::abs(e.x - best_col)) {
                        best_col = c;
                    }
                }
                if (std::abs(e.x - best_col) > (row.size() == 1 ? 25 : 45)) {
                    continue;
                }
                if (std::ranges::find(cols_hit, best_col) == cols_hit.end()) {
                    cols_hit.emplace_back(best_col);
                }
            }
            if (cols_hit.empty()) {
                continue;
            }
            double ry = 0;
            for (const auto& e : row) {
                ry += e.y;
            }
            ry /= row.size();
            for (int c : cols_hit) {
                cells.emplace_back(c, static_cast<int>(std::lround(ry)));
            }
        }
        std::ranges::sort(cells, [](const Point& a, const Point& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
        return cells;
    };

    const auto scroll_task = Task.get("StrongholdProtocol@StrategyListScroll");
    auto scroll = [&](bool toward_top) {
        if (need_exit() || !scroll_task || scroll_task->special_params.size() < 4) {
            return false;
        }
        const auto& params = scroll_task->special_params;
        const auto& from = scroll_task->specific_rect;
        const auto& to = scroll_task->rect_move;
        return ctrler()->swipe(
                   toward_top ? to : from,
                   toward_top ? from : to,
                   params[0],
                   to_swipe_extra_direction(params[1]),
                   params[2] / 10.0,
                   params[3] / 10.0,
                   false,
                   scroll_task->high_resolution_swipe_fix) &&
               sleep(scroll_task->post_delay);
    };

    constexpr int max_pages = 8;
    auto image = ctrler()->get_image();
    auto thumb = scrollbar(image);
    for (int page = 0; thumb && thumb->y > track_top + 2 && page < max_pages; ++page) {
        const int previous_top = thumb->y;
        const bool swiped = scroll(true);
        image = ctrler()->get_image();
        thumb = scrollbar(image);
        if (!swiped || !thumb || thumb->y >= previous_top) {
            LogWarn << "could not return to the start of the strategy list";
            break;
        }
    }
    bool verified = thumb && std::abs(thumb->y - track_top) <= 2;
    std::vector<std::string> tried;
    std::vector<Point> covered_top;
    for (int page = 0; page < max_pages && !need_exit(); ++page) {
        if (image.cols < 1280 || image.rows < 720) {
            LogWarn << "strategy screenshot unavailable";
            return StrategyResult::IncompleteScan;
        }
        std::vector<Rect> completed;
        auto cells = detect_grid_cells(image, completed);
        if (thumb) {
            auto frames = detect_card_frames(image, *thumb, verified);
            verified &= !frames.empty();
            // 保留未能核对卡框的立绘候选。裁切到视窗边缘的卡由相邻页覆盖，侧栏不属于网格。
            const int bar_x = thumb->x + thumb->width / 2;
            std::erase_if(cells, [&](const Point& pt) {
                const auto same_cell = [&](const Point& center) {
                    return std::abs(pt.x - center.x) < 25 && std::abs(pt.y - center.y) < 45;
                };
                return pt.x < bar_x + frame.candidate_left || pt.x > bar_x ||
                       (!frames.empty() && ((pt.y < frame.candidate_y[0] && thumb->y > track_top + 2) ||
                                            (pt.y > frame.candidate_y[1] && thumb->br().y < track_bottom - 2))) ||
                       std::ranges::any_of(frames, same_cell) || std::ranges::any_of(covered_top, same_cell);
            });
            // 只有卡框能证明这一页的卡已核全。仅靠标记或立绘定位的卡仍可选择。
            verified &= cells.empty();
            cells.insert(cells.end(), frames.begin(), frames.end());
            std::ranges::sort(cells, [](const Point& a, const Point& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
        }
        if (cells.empty()) {
            // 一格都找不到是识别异常（区别于全达标），存图留证
            save_img(utils::path("debug") / utils::path("stronghold"));
            LogWarn << "strategy grid not detected, page" << page;
            return StrategyResult::IncompleteScan;
        }
        int grid_offset = 0;
        for (const auto& pt : cells) {
            if (need_exit()) {
                return StrategyResult::IncompleteScan;
            }
            if (std::ranges::any_of(completed, [&](const Rect& cell) { return cell.include(pt); })) {
                continue;
            }

            // 点击可能留在上一张卡的详情。面板读数可用于挑选，只有当前卡的角标能独立证明它已达标。
            verified = false;
            if (!ctrler()->click(Point { pt.x + grid_offset, pt.y }) || !sleep(400)) {
                return StrategyResult::IncompleteScan;
            }

            // 详情切换有动画，姓名和通过次数要在同一帧读清。
            std::string strategy_name;
            int passed = 0;
            for (int attempt = 0; attempt < 3 && !need_exit(); ++attempt) {
                const auto panel = ctrler()->get_image();
                if (thumb) {
                    if (const auto panel_thumb = scrollbar(panel)) {
                        const int next_offset = panel_thumb->x - thumb->x;
                        // 再点已选中的卡会收起详情，按网格的新位置重新点同一张卡。
                        if (next_offset > grid_offset + 100) {
                            grid_offset = next_offset;
                            if (!ctrler()->click(Point { pt.x + grid_offset, pt.y }) || !sleep(400)) {
                                return StrategyResult::IncompleteScan;
                            }
                            continue;
                        }
                        grid_offset = next_offset;
                    }
                }
                OCRer name(panel);
                name.set_task_info("StrongholdProtocol@StrategyName");
                if (const auto rows = name.analyze(); rows && !rows->front().text.empty()) {
                    if (const auto count = read_strategy_passed(panel)) {
                        strategy_name = rows->front().text;
                        passed = *count;
                        break;
                    }
                }
                sleep(300);
            }
            if (strategy_name.empty()) {
                LogWarn << "strategy panel unreadable, skip grid" << pt.x << pt.y;
                continue;
            }
            if (std::ranges::find(tried, strategy_name) != tried.end()) {
                continue;
            }
            tried.emplace_back(strategy_name);
            if (passed >= m_target) {
                continue;
            }

            Matcher confirm(ctrler()->get_image());
            confirm.set_task_info("StrongholdProtocol@SPRConfirmSelect");
            const auto button = confirm.analyze();
            if (need_exit() || !button || !ctrler()->click(button->rect)) {
                LogWarn << "strategy confirmation unavailable:" << strategy_name;
                continue;
            }
            // 歌利亚卡片滚出屏幕后也会失配。确认时检查实际进入的后续场景。
            Scene scene = Scene::Unknown;
            for (int attempt = 0; attempt < 12; ++attempt) {
                if (!sleep(200)) {
                    return StrategyResult::IncompleteScan;
                }
                scene = detect_scene(ctrler()->get_image());
                if ((scene == Scene::CustomSquad || scene == Scene::BountySelect || scene == Scene::ConfirmInfo ||
                     scene == Scene::RoundPrepare) &&
                    !need_exit()) {
                    next_scene = scene;
                    m_current_strategy = strategy_name;
                    LogInfo << "select strategy:" << strategy_name << "passed" << passed << "target" << m_target;
                    auto info = basic_info_with_what("StrongholdStrategy");
                    info["details"]["name"] = strategy_name;
                    info["details"]["passed"] = passed;
                    info["details"]["target"] = m_target;
                    callback(AsstMsg::SubTaskExtraInfo, info);
                    return StrategyResult::Selected;
                }
            }
            LogWarn << "strategy confirmation did not reach the next scene:" << strategy_name;
            if (scene != Scene::StrategySelect) {
                return StrategyResult::IncompleteScan;
            }
        }
        if (thumb && std::abs(thumb->br().y - track_bottom) <= 2) {
            if (!verified) {
                LogWarn << "reached the end of the strategy list with unverified cards";
            }
            return verified && !need_exit() ? StrategyResult::AllCompleted : StrategyResult::IncompleteScan;
        }
        if (page + 1 == max_pages || !scroll(false)) {
            break;
        }
        auto after = ctrler()->get_image();
        auto next_thumb = scrollbar(after);
        covered_top.clear();
        if (thumb && next_thumb) {
            const int advance = next_thumb->y - thumb->y;
            if (advance <= 0) {
                LogWarn << "strategy list did not advance" << advance;
                return StrategyResult::IncompleteScan;
            }
            if (verified) {
                // 滑块先到位时卡片可能还停在原页。用上一页底部一张卡高的内容确认两页确实重叠。
                const int overlap_height = frame.height[1];
                const int previous_left = thumb->x + thumb->width / 2 + view.x;
                const int next_left = next_thumb->x + next_thumb->width / 2 + view.x;
                const int last_y = view.height - overlap_height;
                const int min_shift = std::max(1, advance * (track_bottom - track_top) / thumb->height - 4);
                const int max_shift = std::min(last_y, advance * view.height / thumb->height + 4);
                verified = std::abs(next_thumb->height - thumb->height) <= 2 && min_shift <= max_shift;
                if (verified) {
                    const cv::Mat strip = image(cv::Rect(previous_left, view.y + last_y, view.width, overlap_height));
                    const cv::Mat grid = after(cv::Rect(next_left, view.y, view.width, view.height));
                    cv::Mat scores;
                    cv::matchTemplate(grid, strip, scores, cv::TM_CCOEFF_NORMED);
                    double moved = 0;
                    cv::Point match;
                    cv::minMaxLoc(
                        scores.rowRange(last_y - max_shift, last_y - min_shift + 1),
                        nullptr,
                        &moved,
                        nullptr,
                        &match);
                    verified = moved >= 0.8 && moved > scores.at<float>(last_y, 0) + 0.001;
                    if (verified) {
                        // 仅复用上页已核全的卡框。按实测位移映射到新页后，顶部被裁断的角标无需再识别。
                        const int shift = max_shift - match.y;
                        for (const auto& cell : cells) {
                            const int y = cell.y - shift;
                            if (y >= view.y && y <= view.y + overlap_height + frame.center_from_bottom_left[1]) {
                                covered_top.emplace_back(cell.x + next_left - previous_left, y);
                            }
                        }
                    }
                }
                if (!verified) {
                    LogWarn << "strategy list page overlap unconfirmed" << advance;
                }
            }
        }
        else {
            verified = false;
            if (after.size() != image.size()) {
                return StrategyResult::IncompleteScan;
            }
            const auto shift = measure_grid_shift(image, after);
            if (shift && *shift <= 0) {
                LogWarn << "strategy list did not advance" << *shift;
                return StrategyResult::IncompleteScan;
            }
        }
        image = std::move(after);
        thumb = next_thumb;
    }
    LogWarn << "strategy list scan stopped before the end";
    return StrategyResult::IncompleteScan;
}

bool StrongholdProtocolTaskPlugin::pick_squad(BoardState& board)
{
    LogTraceFunction;

    const auto confirm = "StrongholdProtocol@CustomOpersConfirm";
    if (board.squad_picked) {
        return click_template(confirm);
    }

    const int offset = task_roi("StrongholdProtocol@Roi@SquadNameOffset").y;
    std::set<std::string> attempted;
    for (const int tier : { 5, 6 }) {
        const std::string row = tier == 5 ? "V" : "VI";
        auto image = ctrler()->get_image();
        OCRer ocr(image);
        ocr.set_task_info("StrongholdProtocol@SquadOpers" + row);
        const auto text = ocr.analyze();
        if (!text) {
            continue;
        }
        OCRer disabled(image);
        disabled.set_task_info("StrongholdProtocol@SquadDisabled" + row);
        const auto marks = disabled.analyze();
        std::vector<TextRect> names;
        for (const auto& tr : *text) {
            if (!BattleData.get_all_oper_names().contains(tr.text) || StrongholdProtocol.is_substitute(tr.text) ||
                StrongholdProtocol.is_squad_blacklisted(tr.text) || !StrongholdProtocol.is_six_star(tr.text)) {
                continue;
            }
            const int x = tr.rect.x + tr.rect.width / 2;
            const bool banned = marks && std::ranges::any_of(*marks, [&](const TextRect& mark) {
                                    return mark.rect.y > tr.rect.y && mark.rect.y - tr.rect.y < 80 &&
                                           std::abs(x - mark.rect.x - mark.rect.width / 2) < 60;
                                });
            if (!banned) {
                names.emplace_back(tr);
            }
        }
        std::ranges::sort(names, {}, [](const TextRect& tr) {
            return std::pair { -StrongholdProtocol.priority(tr.text, true), tr.rect.x };
        });
        const std::string counter = "StrongholdProtocol@SquadCount" + row;
        int used_slots = 0;
        for (const auto& tr : names) {
            if (used_slots == 2 || need_exit()) {
                break;
            }
            if (!attempted.emplace(tr.text).second) {
                continue;
            }
            const auto before = read_number(ctrler()->get_image(), counter);
            if (before == 2) {
                break;
            }
            if (!ctrler()->click(Point(tr.rect.x + tr.rect.width / 2, tr.rect.y + tr.rect.height / 2 + offset))) {
                continue;
            }
            // 同一名牌再次点击会取消选择。只有分子递增才记录，读数不明时给这次点击预留一个名额。
            std::optional<int> after;
            for (int retry = 0; retry < 3 && !need_exit(); ++retry) {
                sleep(200);
                after = read_number(ctrler()->get_image(), counter);
                if (before && after && before != after) {
                    break;
                }
            }
            if ((before == 0 || before == 1) && after == *before + 1) {
                board.squad.emplace(tr.text, tier);
                ++used_slots;
                LogInfo << __FUNCTION__ << "selected" << tr.text << "tier" << row << "count" << *after;
            }
            else {
                if (!before || !after || *before > 2 || *after > 2 || *after > *before) {
                    ++used_slots;
                }
                LogWarn << __FUNCTION__ << "selection unconfirmed" << tr.text << "tier" << row << "count"
                        << before.value_or(-1) << "->" << after.value_or(-1);
            }
        }
    }
    // 确认失败或过场尚未结束时，下次只补点确认，避免再次点击头像取消选择。
    board.squad_picked = true;
    return !need_exit() && click_template(confirm);
}

// ---------------- 局内：商店读取与操作 ----------------

std::optional<int> StrongholdProtocolTaskPlugin::read_number(
    const cv::Mat& image,
    const std::string& task_name,
    const std::optional<Rect>& roi) const
{
    // 任务自身的 [^\d] 剔除表会整体覆盖 baseTask NumberOcrReplace 的纠错表（baseTask 不合并数组），
    // 串联回纠错表让 O→0、l→1 等误识别先纠正再剔除，而不是被删掉
    const auto task = Task.get<OcrTaskInfo>(task_name);
    if (!task || roi.value_or(task->roi).empty()) {
        return std::nullopt;
    }
    const auto& number_replace = Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map;
    const auto& task_replace = task->replace_map;
    std::decay_t<decltype(task_replace)> merged_replace {};
    merged_replace.reserve(number_replace.size() + task_replace.size());
    std::ranges::copy(number_replace, std::back_inserter(merged_replace));
    std::ranges::copy(task_replace, std::back_inserter(merged_replace));

    RegionOCRer ocr(image);
    ocr.set_task_info(task);
    if (roi) {
        ocr.set_roi(*roi);
    }
    ocr.set_replace(merged_replace);
    ocr.set_use_raw(false);
    int value = 0;
    if (!ocr.analyze() || !utils::chars_to_number(ocr.get_result().text, value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> StrongholdProtocolTaskPlugin::read_gold(const cv::Mat& image)
{
    // 读失败换新截图重试，并用加宽识别框兜底；仍失败返回 nullopt（未知，不是 0）
    constexpr int tries = 3;
    for (int attempt = 0; attempt < tries && !need_exit(); ++attempt) {
        const cv::Mat img = attempt == 0 ? image : ctrler()->get_image();
        auto gold = read_number(img, "StrongholdProtocol@ShopGoldOcr");
        if (!gold) {
            gold = read_number(img, "StrongholdProtocol@ShopGoldWideOcr");
        }
        if (gold && *gold >= 0 && *gold <= 99) {
            return gold;
        }
        if (attempt + 1 < tries) {
            sleep(400);
        }
    }
    LogWarn << __FUNCTION__ << "gold unreadable, treated as unknown";
    return std::nullopt;
}

StrongholdProtocolTaskPlugin::ShopInfo StrongholdProtocolTaskPlugin::read_shop(const cv::Mat& image, BoardState& board)
{
    ShopInfo info;
    info.gold = read_gold(image);
    info.round = read_number(image, "StrongholdProtocol@ShopRoundOcr");
    if (board.maxed_shop_x) {
        info.maxed = true;
        info.upgrade_x = board.maxed_shop_x;
        m_upgrade_x = info.upgrade_x;
        return info;
    }

    // 升级按钮随商店等级移动：取 ｢升级｣/｢满级｣ 文字中心 x 定位，命中 ｢满级｣ 即商店已满
    OCRer bottom(image);
    bottom.set_task_info("StrongholdProtocol@ShopBottomOcr");
    if (auto results = bottom.analyze(); results && !results->empty()) {
        const auto& button = results->front();
        info.upgrade_x = button.rect.x + button.rect.width / 2;
        for (const auto& tr : *results) {
            if (tr.text == "maxed") {
                info.maxed = true;
                info.upgrade_x = tr.rect.x + tr.rect.width / 2;
                board.maxed_shop_x = info.upgrade_x;
                break;
            }
        }
    }
    m_upgrade_x = info.upgrade_x; // 商店卡牌识别据此剔除按钮区
    if (!info.maxed) {
        // 费用徽章在按钮正上方，按按钮位置读；定位失败或没读到时用任务自身的宽 roi 兜底
        if (info.upgrade_x) {
            const Rect off = task_roi("StrongholdProtocol@Roi@ShopUpgradeCost");
            info.upgrade_cost = read_number(
                image,
                "StrongholdProtocol@ShopUpgradeCostOcr",
                Rect { *info.upgrade_x + off.x, off.y, off.width, off.height });
        }
        if (!info.upgrade_cost) {
            info.upgrade_cost = read_number(image, "StrongholdProtocol@ShopUpgradeCostOcr");
        }
    }
    return info;
}

static std::vector<Rect> locate_tier_badges(const cv::Mat& image, const Rect& scan, int width, int height)
{
    const auto task = Task.get("StrongholdProtocol@TierBadgeLocate");
    const auto roi = make_rect<cv::Rect>(scan) & cv::Rect(0, 0, image.cols, image.rows);
    if (!task || task->special_params.size() != 7 || roi.empty() || image.type() != CV_8UC3) {
        return {};
    }
    const auto& params = task->special_params;
    const int dark_limit = params[0], rim = params[1], size_tolerance = params[2];
    const double min_area = params[3] / 100.0;
    const int inset = params[4], min_contrast = params[5];
    const double min_ink = params[6] / 100.0;
    if (rim < 0 || inset < 0 || width <= 2 * rim || height <= 2 * rim) {
        return {};
    }

    cv::Mat gray, dark;
    cv::cvtColor(image(roi), gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, dark, dark_limit, 255, cv::THRESH_BINARY_INV);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(dark, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    std::vector<Rect> badges;
    for (const auto& contour : contours) {
        const auto face = cv::boundingRect(contour);
        if (std::abs(face.width - (width - 2 * rim)) > size_tolerance ||
            std::abs(face.height - (height - 2 * rim)) > size_tolerance ||
            cv::contourArea(contour) < face.area() * min_area || face.width <= 2 * inset || face.height <= 2 * inset) {
            continue;
        }
        // 同一灰色字在阴影下会变暗，按牌内的明暗差保留可点选的席位。
        const cv::Rect inner(face.x + inset, face.y + inset, face.width - 2 * inset, face.height - 2 * inset);
        if (cv::countNonZero(dark(inner)) == 0) {
            continue;
        }
        const auto background = cv::mean(gray(inner), dark(inner))[0];
        if (cv::countNonZero(gray(inner) > background + min_contrast) < inner.area() * min_ink) {
            continue;
        }
        const Rect frame { roi.x + face.x + face.width / 2 - width / 2,
                           roi.y + face.y + face.height / 2 - height / 2,
                           width,
                           height };
        if (scan.include(frame) && std::ranges::none_of(badges, [&](const Rect& other) {
                return std::abs(other.x - frame.x) <= size_tolerance && std::abs(other.y - frame.y) <= size_tolerance;
            })) {
            badges.emplace_back(frame);
        }
    }
    return badges;
}

std::vector<StrongholdProtocolTaskPlugin::ShopCard>
    StrongholdProtocolTaskPlugin::read_shop_cards(const cv::Mat& image, const std::vector<ShopCard>& previous) const
{
    OCRer ocr(image);
    ocr.set_task_info("StrongholdProtocol@ShopCardsOcr");
    auto results = ocr.analyze().value_or(OCRer::ResultsVec {});
    OCRer names(image);
    names.set_task_info("StrongholdProtocol@ShopCardNamesOcr");
    if (auto rows = names.analyze()) {
        results.insert(results.end(), std::make_move_iterator(rows->begin()), std::make_move_iterator(rows->end()));
    }

    const auto& oper_names = BattleData.get_all_oper_names();
    const auto parse_cost = [](std::string_view text) -> std::optional<int> {
        unsigned value = 0;
        if (text.size() <= 2 && utils::chars_to_number<unsigned, true>(text, value)) {
            return static_cast<int>(value);
        }
        return std::nullopt;
    };
    const auto frame_task = Task.get("StrongholdProtocol@Roi@ShopTierFrame");
    if (results.empty() || !frame_task || frame_task->special_params.size() != 3) {
        return {};
    }
    const auto& tier_frame = frame_task->roi;
    const int merge_distance = frame_task->special_params[0];
    const int text_distance = frame_task->special_params[1];
    const int name_distance = frame_task->special_params[2];
    const auto cost_roi = task_roi("StrongholdProtocol@Roi@ShopCostY");
    std::vector<ShopCard> cards;
    auto center = [](const Rect& r) {
        return r.x + r.width / 2;
    };
    auto center_y = [](const Rect& r) {
        return r.y + r.height / 2;
    };
    const auto empty_roi = task_roi("StrongholdProtocol@Roi@ShopEmptyY");
    const auto pact_roi = task_roi("StrongholdProtocol@Roi@ShopPactY");
    const auto name_roi2 = task_roi("StrongholdProtocol@Roi@ShopNameY");
    const auto cost_reread = task_roi("StrongholdProtocol@Roi@ShopCostReread");
    const auto card_rect = [](const Rect& r, int card_x) {
        return Rect { card_x + r.x, r.y, r.width, r.height };
    };
    const auto read_card_cost = [&](int x) {
        if (cost_reread.empty()) {
            return -1;
        }
        RegionOCRer cost(image);
        cost.set_task_info("StrongholdProtocol@Roi@ShopCostReread");
        cost.set_roi(card_rect(cost_reread, x));
        const auto result = cost.analyze();
        return result ? parse_cost(result->text).value_or(-1) : -1;
    };
    const int cost_lo = cost_roi.y, cost_hi = cost_roi.y + cost_roi.height;
    const auto shelf = task_roi("StrongholdProtocol@ShopCardsOcr");
    const Rect strip { shelf.x, tier_frame.y, shelf.width, tier_frame.height };
    std::vector<int> card_xs;
    for (const auto& frame : locate_tier_badges(image, strip, tier_frame.width, tier_frame.height)) {
        card_xs.emplace_back(frame.x - tier_frame.x);
    }
    // 费用徽章在卡槽中央，可补全等阶方牌被遮住的卡位。
    for (const auto& row : results) {
        const int x = center(row.rect);
        const int y = center_y(row.rect);
        if (y >= cost_lo && y <= cost_hi &&
            std::ranges::none_of(card_xs, [&](int current) { return std::abs(current - x) <= merge_distance; })) {
            card_xs.emplace_back(x);
        }
    }
    const int empty_lo = empty_roi.y, empty_hi = empty_roi.y + empty_roi.height;
    const int pact_lo = pact_roi.y, pact_hi = pact_roi.y + pact_roi.height;
    const int name_lo2 = name_roi2.y, name_hi2 = name_roi2.y + name_roi2.height;
    std::map<int, std::string> recovered;
    if (!previous.empty()) {
        // 费用和方牌都漏检时，仍可用全名的位移修正上次确认的卡槽中心。
        for (const auto& old : previous) {
            if (!old.is_oper || old.cost <= 0 || old.name_left < 0 || old.name.empty()) {
                continue;
            }
            for (const auto& tr : results) {
                const int dx = tr.rect.x - old.name_left;
                const int cy = center_y(tr.rect);
                if (tr.text != old.name || std::abs(dx) > name_distance || cy < name_lo2 || cy > name_hi2) {
                    continue;
                }
                const int x = old.x + dx;
                if (std::ranges::none_of(card_xs, [&](int current) {
                        return std::abs(current - x) <= text_distance;
                    })) {
                    recovered.emplace(x, old.name);
                    card_xs.emplace_back(x);
                }
                break;
            }
        }
    }
    std::ranges::sort(card_xs);
    if (m_upgrade_x) {
        const int span = task_roi("StrongholdProtocol@Roi@ShopUpgradeButtonSpan").width;
        std::erase_if(card_xs, [&](int x) { return x <= *m_upgrade_x + span; });
    }
    for (int x : card_xs) {
        ShopCard card;
        card.x = x;
        for (const auto& tr : results) {
            const int cx = center(tr.rect);
            const int cy = center_y(tr.rect);
            if (std::abs(cx - x) > text_distance) {
                continue;
            }
            LogTrace << "shop tr card" << x << "text" << tr.text << "cx" << cx << "cy" << cy;
            if (cy >= cost_lo && cy <= cost_hi && std::abs(cx - x) <= cost_reread.width / 2) {
                if (const auto cost = parse_cost(tr.text)) {
                    card.cost = *cost;
                    continue;
                }
            }
            if (cy >= empty_lo && cy <= empty_hi && tr.text == "empty") {
                card.empty = true;
                continue;
            }
            if (cy >= pact_lo && cy <= pact_hi && tr.text == "pact") {
                card.is_oper = true;
                continue;
            }
            if (cy >= name_lo2 && cy <= name_hi2 && card.name.empty() && oper_names.contains(tr.text)) {
                card.name = tr.text;
                card.name_left = tr.rect.x;
                LogTrace << "shop name matched" << tr.text << "card" << x;
            }
        }
        if (const auto it = recovered.find(x); it != recovered.end()) {
            if (card.empty || !card.is_oper || card.name != it->second) {
                continue;
            }
            card.cost = read_card_cost(x);
            if (card.cost <= 0) {
                continue;
            }
            LogTrace << "recovered shop slot" << x << card.name << "cost" << card.cost;
        }
        else if (card.cost < 0 && !card.empty) {
            card.cost = read_card_cost(x);
        }
        if (!card.empty) {
            card.tier = stronghold::match_tier_badge(image, card_rect(tier_frame, x));
        }
        card.is_oper = card.is_oper && !card.empty;
        cards.emplace_back(card);
        LogTrace << "shop card x" << x << "cost" << card.cost << "tier" << card.tier << "oper" << card.is_oper
                 << "empty" << card.empty << "name" << card.name;
    }
    // 普通商店的费用徽章可能只读出一小段像 0 的笔画；混有付费卡时，单独的 0 需要补读。
    if (std::ranges::any_of(cards, [](const ShopCard& c) { return !c.empty && c.cost > 0; }) &&
        !is_reward_shop(cards)) {
        for (auto& card : cards) {
            if (card.is_oper && card.cost == 0) {
                const int cost = read_card_cost(card.x);
                card.cost = cost > 0 ? cost : -1;
                LogTrace << "rechecked isolated zero cost" << card.x << "cost" << card.cost;
            }
        }
    }
    return cards;
}

std::optional<int> StrongholdProtocolTaskPlugin::field_medic_tier(const BoardState& board)
{
    if (std::ranges::any_of(board.unconfirmed_tiles, [](const auto& entry) { return is_medic(entry.second); })) {
        return 0;
    }
    std::optional<int> tier;
    for (const auto& [key, occupant] : board.occ) {
        if (is_medic(occupant.first)) {
            // 合成可能腾出该格，在部署预览确认之前保留占位。
            if (tier || board.promotion_tiles.contains(key) || key == "path0") {
                return 0;
            }
            tier = occupant.second;
        }
    }
    return tier;
}

static bool improves_field_medic(std::optional<int> field, int tier)
{
    return !field || (*field > 0 && *field < 5 && tier > *field);
}

bool StrongholdProtocolTaskPlugin::can_buy_card(
    const BoardState& board,
    const ShopCard& card,
    ShopPhase phase,
    int slots)
{
    if (phase == ShopPhase::PromotionReward) {
        return true;
    }
    const auto role = StrongholdProtocol.get_role(card.name);
    if (role == battle::Role::Unknown || role == battle::Role::Drone ||
        (phase == ShopPhase::Maxed && card.cost > 0 && card.tier < 5)) {
        return false;
    }
    if (card.cost > 0) {
        const auto copies = board.purchases.find(card.name);
        if (copies != board.purchases.end() && copies->second.unmerged == 2) {
            return true;
        }
    }
    if (role == battle::Role::Medic) {
        const auto field = field_medic_tier(board);
        if (!improves_field_medic(field, card.tier) || std::ranges::any_of(board.bench_known, [&](const auto& copy) {
                return is_medic(copy.first) && improves_field_medic(field, copy.second);
            })) {
            return false;
        }
    }
    return phase == ShopPhase::Opening || card.cost == 0 ||
           choose_tile(board, card.name, { card.tier, false }, {}, slots).has_value();
}

bool StrongholdProtocolTaskPlugin::is_reward_shop(const std::vector<ShopCard>& cards)
{
    return std::ranges::count_if(cards, [](const ShopCard& card) {
               return card.is_oper && !card.empty && card.cost == 0;
           }) == 3;
}

bool StrongholdProtocolTaskPlugin::may_have_bench_elite(const BoardState& board, const std::string& name)
{
    const auto bought = board.purchases.find(name);
    if (bought == board.purchases.end() || bought->second.promotions == 0) {
        return false;
    }
    const auto on_field = std::ranges::count_if(board.elite_tiles, [&](const std::string& key) {
        const auto occupant = board.occ.find(key);
        return occupant != board.occ.end() && occupant->second.first == name;
    });
    return bought->second.promotions > on_field;
}

void StrongholdProtocolTaskPlugin::record_promotion(
    BoardState& board,
    const ShopCard& card,
    std::optional<int> slots_freed)
{
    if (card.name.empty()) {
        return;
    }
    const bool previous_elites_located = !may_have_bench_elite(board, card.name);
    auto& copies = board.purchases.at(card.name);
    ++copies.promotions;
    copies.unmerged = std::max(0, copies.unmerged - 3);
    std::vector<std::string> materials;
    for (const auto& [key, occupant] : board.occ) {
        if (occupant.first == card.name && !board.elite_tiles.contains(key)) {
            materials.push_back(key);
            board.promotion_tiles.emplace(key);
        }
    }
    // 只有此前的精锐都已定位时，才知道同名备战席记录属于本次合成素材。
    if (previous_elites_located) {
        std::erase_if(board.bench_known, [&](const auto& copy) { return copy.first == card.name; });
        board.displaced.erase(card.name);
    }
    const int count = static_cast<int>(materials.size());
    if (previous_elites_located && board.unconfirmed_tiles.empty() && !board.unknown_displaced && count <= 2 &&
        slots_freed) {
        if (*slots_freed == count) {
            for (const auto& key : materials) {
                board.occ.erase(key);
                board.promotion_tiles.erase(key);
            }
        }
        else if (count == 1 && *slots_freed == 0) {
            board.elite_tiles.emplace(materials.front());
            board.promotion_tiles.erase(materials.front());
            return;
        }
        else if (count == 2 && *slots_freed == 1) {
            board.promotion_partners[materials[0]] = materials[1];
            board.promotion_partners[materials[1]] = materials[0];
            return;
        }
    }
    board.bench_known.emplace_back(card.name, card.tier);
}

void StrongholdProtocolTaskPlugin::resolve_promotion_tile(BoardState& board, const std::string& key, bool occupied)
{
    if (board.promotion_tiles.erase(key) == 0) {
        return;
    }
    if (const auto it = board.promotion_partners.find(key); it != board.promotion_partners.end()) {
        const auto other = it->second;
        board.promotion_partners.erase(it);
        board.promotion_partners.erase(other);
        if (!board.unconfirmed_tiles.contains(other)) {
            board.elite_tiles.emplace(occupied ? key : other);
            board.promotion_tiles.erase(other);
            board.occ.erase(occupied ? other : key);
        }
    }
    if (!occupied) {
        board.occ.erase(key);
    }
}

std::optional<bool> StrongholdProtocolTaskPlugin::panel_is_elite(const cv::Mat& preview, int tier)
{
    if (tier < 1 || tier > 6) {
        return std::nullopt;
    }
    // 同一 LV 在不同活动等阶可能对应不同的精锐状态。
    const auto ocr = Task.get<OcrTaskInfo>("StrongholdProtocol@DragLevelOcr");
    const auto elite = Task.get<OcrTaskInfo>("StrongholdProtocol@DragEliteLevels");
    const auto initial = Task.get<OcrTaskInfo>("StrongholdProtocol@DragInitialLevels");
    const auto index = static_cast<size_t>(tier - 1);
    if (!ocr || ocr->special_params.empty() || ocr->special_params.front() < 0 || ocr->special_params.front() > 100 ||
        !elite || index >= elite->text.size() || !initial || index >= initial->text.size()) {
        return std::nullopt;
    }
    OCRer level(preview);
    level.set_task_info(ocr);
    const auto rows = level.analyze();
    if (rows && rows->size() == 1 && rows->front().score >= ocr->special_params.front() / 100.0) {
        if (rows->front().text == elite->text[index]) {
            return true;
        }
        if (rows->front().text == initial->text[index]) {
            return false;
        }
    }
    return std::nullopt;
}

std::optional<StrongholdProtocolTaskPlugin::ShopCard>
    StrongholdProtocolTaskPlugin::promotion_reward(const std::vector<ShopCard>& cards, const BoardState& board)
{
    return is_reward_shop(cards) ? shop_pick(cards, board, 0, -1) : std::nullopt;
}

std::optional<StrongholdProtocolTaskPlugin::ShopCard> StrongholdProtocolTaskPlugin::shop_pick(
    const std::vector<ShopCard>& cards,
    const BoardState& board,
    int gold,
    int slots)
{
    enum class Group
    {
        Low,
        Five,
        Substitute,
        Six,
        SquadFive,
        SquadSix,
        Priority
    };
    const auto rank = [&](const ShopCard& card) {
        if (card.tier > 0 && card.tier < 5) {
            return std::pair { Group::Low, card.tier };
        }
        if (const int priority = StrongholdProtocol.priority(card.name)) {
            return std::pair { Group::Priority, priority };
        }
        if (const auto it = board.squad.find(card.name); it != board.squad.end()) {
            // 自选档取编队页的行等阶，卡面实测等阶另用于付费筛选和部署。
            return std::pair { it->second == 6 ? Group::SquadSix : Group::SquadFive, 0 };
        }
        if (card.tier == 0) {
            return std::pair { Group::Low, 0 };
        }
        if (StrongholdProtocol.is_substitute(card.name)) {
            return std::pair { Group::Substitute, 0 };
        }
        return std::pair { card.tier == 6 ? Group::Six : Group::Five, 0 };
    };
    std::optional<ShopCard> best;
    const bool missing_medic = !field_medic_tier(board);
    std::tuple<bool, bool, std::pair<Group, int>, bool, int> best_score;
    for (const auto& card : cards) {
        const bool free = card.cost == 0;
        if (!card.is_oper || card.empty || card.cost < 0 || card.cost > gold || (!free && card.tier < 5)) {
            continue;
        }
        const bool suitable = can_buy_card(board, card, ShopPhase::Maxed, slots);
        if (!free && !card.name.empty() && !suitable) {
            continue;
        }
        const auto copies = board.purchases.find(card.name);
        // 免费三选一必须领走一张。有合适的赠卡时才跳过多余的医疗。
        const auto score = std::tuple { free,
                                        !free || suitable,
                                        rank(card),
                                        missing_medic && suitable && is_medic(card.name),
                                        copies == board.purchases.end() ? 0 : copies->second.unmerged };
        if (!best || score > best_score) {
            best = card;
            best_score = score;
        }
    }
    return best;
}

static int panel_tier(const cv::Mat& preview)
{
    return stronghold::match_tier_badge(preview, task_roi("StrongholdProtocol@Roi@DragTierFrame"));
}

StrongholdProtocolTaskPlugin::PurchaseResult StrongholdProtocolTaskPlugin::buy_oper(
    ShopCard& card,
    const cv::Mat& before,
    cv::Mat& after,
    std::vector<ShopCard>& cards_after,
    ShopPhase phase,
    const BoardState& board)
{
    const bool reward = phase == ShopPhase::PromotionReward;
    const int slots = card.cost > 0 && phase != ShopPhase::Opening ? slots_left(before).value_or(-1) : -1;
    if (!card.name.empty() && !can_buy_card(board, card, phase, slots)) {
        return PurchaseResult::Skipped;
    }
    const auto gold0 = card.cost == 0 ? std::nullopt : read_gold(before);
    const int card_y = task_roi("StrongholdProtocol@Roi@ShopCardY").y;
    ctrler()->click(Point(card.x, card_y));

    // 等 ｢资金不足｣ 提示或左侧特质面板出现；任一命中即停止轮询（≤1.2s）
    bool saw_no_funds = false;
    bool saw_trait = false;
    for (int i = 0; i < 6 && !need_exit(); ++i) {
        sleep(200);
        const auto img = ctrler()->get_image();
        OCRer probe(img);
        probe.set_task_info("StrongholdProtocol@ShopNoFundsOcr");
        if (probe.analyze()) {
            saw_no_funds = true;
            break;
        }
        OCRer trait(img);
        trait.set_task_info("StrongholdProtocol@ShopTraitOcr");
        if (trait.analyze()) {
            saw_trait = true;
            if (const int tier = panel_tier(img)) {
                card.tier = tier;
            }
            if (const auto task = Task.get<OcrTaskInfo>("StrongholdProtocol@DragNameOcr");
                task && !task->special_params.empty()) {
                OCRer name(img);
                name.set_task_info(task);
                if (const auto rows = name.analyze(); rows &&
                                                      rows->front().score >= task->special_params.front() / 100.0 &&
                                                      BattleData.get_all_oper_names().contains(rows->front().text)) {
                    card.name = rows->front().text;
                }
            }
            if (card.name.empty() && !reward) {
                continue;
            }
            break;
        }
    }
    if (saw_no_funds) {
        ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
        return PurchaseResult::Failed;
    }
    if (!saw_trait || !can_buy_card(board, card, phase, slots)) {
        LogInfo << __FUNCTION__ << "canceling shop selection" << card.name << "at" << card.x;
        ctrler()->click(Point(card.x, card_y));
        ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
        return saw_trait ? PurchaseResult::Skipped : PurchaseResult::Failed;
    }
    // 确认购买：点卡牌下方的购买按钮（y 见 Roi@ShopBuyY，x 跟随卡牌）
    ctrler()->click(Point(card.x, task_roi("StrongholdProtocol@Roi@ShopBuyY").y));

    // 等金币刷新（降价即成功）或资金不足出现（≤1.5s）
    std::optional<int> gold1;
    bool failed = false;
    for (int i = 0; i < 8 && !need_exit(); ++i) {
        sleep(200);
        const auto img = ctrler()->get_image();
        OCRer probe(img);
        probe.set_task_info("StrongholdProtocol@ShopNoFundsOcr");
        if (probe.analyze()) {
            failed = true;
            break;
        }
        if (card.cost == 0) {
            break; // 免费晋升赠送不走资金比对
        }
        gold1 = read_number(img, "StrongholdProtocol@ShopGoldOcr");
        if (gold0 && gold1 && *gold1 < *gold0) {
            break; // 降价即成交
        }
    }
    if (failed || (card.cost != 0 && gold0 && gold1 && *gold1 >= *gold0)) {
        ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
        return PurchaseResult::Failed;
    }
    sleep(600); // 买入动画结束后，备战席和赠送卡槽才同时可读。
    after = ctrler()->get_image();
    cards_after = read_shop_cards(after);
    if (card.cost == 0) {
        // 领取免费卡不会扣钱，须确认原卡已从该槽位消失。
        const bool still_there = std::ranges::any_of(cards_after, [&card](const ShopCard& c) {
            return std::abs(c.x - card.x) < 30 && c.is_oper && c.cost == 0 &&
                   (card.name.empty() || c.name.empty() || card.name == c.name);
        });
        if (still_there) {
            LogWarn << __FUNCTION__ << "free card still in shop, purchase not confirmed:" << card.name;
            ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
            return PurchaseResult::Failed;
        }
    }
    return PurchaseResult::Bought;
}

std::optional<StrongholdProtocolTaskPlugin::ShopCard> StrongholdProtocolTaskPlugin::opening_pick(
    const std::vector<ShopCard>& cards,
    int gold,
    int remaining,
    const BoardState& board)
{
    std::optional<ShopCard> best;
    std::tuple<int, int, int, int> best_score {};
    const auto eligible = [&](const ShopCard& c) {
        return c.is_oper && !c.empty && c.cost > 0 &&
               (c.name.empty() || can_buy_card(board, c, ShopPhase::Opening, -1));
    };
    for (size_t i = 0; i < cards.size(); ++i) {
        const auto& first = cards[i];
        if (!eligible(first)) {
            continue;
        }
        const auto consider = [&](const ShopCard* second) {
            const int cost = first.cost + (second ? second->cost : 0);
            const int medics = is_medic(first.name) + (second && is_medic(second->name));
            if ((gold >= 0 && cost > gold) || medics > 1) {
                return;
            }
            const int melee =
                (opening_front_priority(first.name) > 0) + (second && opening_front_priority(second->name) > 0);
            // 匿名卡可能在详情面板被限购，优先采用两名都已确认的组合。评分相同保留商店顺序。
            const int known = !first.name.empty() + (second && !second->name.empty());
            const auto score = std::tuple { second ? 2 : 1, known, gold < 0 ? -cost : 0, melee };
            if (!best || score > best_score) {
                best = first;
                best_score = score;
            }
        };
        consider(nullptr);
        if (remaining > 1) {
            for (size_t j = i + 1; j < cards.size(); ++j) {
                if (eligible(cards[j])) {
                    consider(&cards[j]);
                }
            }
        }
    }
    return best;
}

StrongholdProtocolTaskPlugin::PurchaseResult StrongholdProtocolTaskPlugin::buy_and_deploy(
    BoardState& board,
    ShopCard card,
    ShopPhase phase,
    std::vector<OpeningOper>* opening)
{
    auto before = ctrler()->get_image();
    if (card.cost == 0) {
        // 领取后其他赠卡会一起消失，不能用上一张商店截图中的 0 费继续购买。
        const auto reward = promotion_reward(read_shop_cards(before), board);
        if (reward) {
            card = *reward;
            phase = ShopPhase::PromotionReward;
        }
        else if (phase == ShopPhase::Early || phase == ShopPhase::Opening) {
            return PurchaseResult::Skipped;
        }
    }
    cv::Mat after;
    auto result = PurchaseResult::Failed;
    while (!need_exit()) {
        std::vector<ShopCard> cards_after;
        const auto purchase = buy_oper(card, before, after, cards_after, phase, board);
        if (purchase != PurchaseResult::Bought) {
            if (purchase == PurchaseResult::Failed) {
                LogWarn << "buy failed:" << card.name << "cost" << card.cost;
            }
            board.pending_deploy |= card.cost == 0;
            return result == PurchaseResult::Bought ? result : purchase;
        }
        result = PurchaseResult::Bought;
        board.bench_roster.clear(); // 合成或买入可能在同一席位换人。
        int count = 0;
        if (!card.name.empty()) {
            auto& copies = board.purchases[card.name];
            count = ++copies.total;
            ++copies.unmerged;
        }
        LogInfo << "bought:" << card.name << "cost" << card.cost << "tier" << card.tier << "purchases" << count;
        if (!is_reward_shop(cards_after)) {
            break;
        }
        // 三个免费干员同时出现表示刚才的购买触发了合成。赠送的干员要另行领取。
        // 同名素材可能已经合成，失去原副本后不能再沿用它的开局格。
        const auto material = [&](const OpeningOper& oper) {
            return card.name.empty() || oper.name == card.name;
        };
        if (opening) {
            std::erase_if(*opening, material);
        }
        const auto slots_before = slots_left(before);
        const auto slots_after = slots_left(after);
        const auto freed = slots_before && slots_after ? std::optional { *slots_after - *slots_before } : std::nullopt;
        record_promotion(board, card, freed);
        const auto reward = shop_pick(cards_after, board, 0, -1);
        LogInfo << "promotion after purchase:" << card.name << "purchases" << count << "reward" << reward->name;
        board.pending_deploy = true;
        before = after;
        card = *reward;
        phase = ShopPhase::PromotionReward;
    }
    if (need_exit()) {
        return result;
    }

    // 当场没部署上的买入干员记下名字与等阶，回合末重扫部署时按名字恢复等阶
    const int tier_hint = card.tier;
    if (!card.name.empty()) {
        board.bench_known.emplace_back(card.name, tier_hint);
    }
    auto slot = new_bench_slot(before, after);
    if (!slot) {
        const auto previous = detect_bench_opers(before);
        auto current = detect_bench_opers(after);
        if (current.size() == previous.size() + 1) {
            for (const auto& old : previous) {
                const auto it =
                    std::ranges::find_if(current, [&](const Point& p) { return Point::distance(p, old) < 12; });
                if (it == current.end()) {
                    break;
                }
                current.erase(it);
            }
            if (current.size() == 1) {
                slot = current.front();
            }
        }
    }
    if (opening) {
        opening->push_back({ card.name, tier_hint, slot, {} });
        return result;
    }
    // 合成后的精锐可能与新卡同时留在备战席，回合末按实际副本排序后再部署。
    if (std::ranges::any_of(board.bench_known, [&](const auto& copy) {
            return may_have_bench_elite(board, copy.first);
        })) {
        board.pending_deploy = true;
        return result;
    }
    if (!is_medic(card.name) && !field_medic_tier(board) &&
        std::ranges::any_of(board.bench_known, [](const auto& copy) { return is_medic(copy.first); })) {
        // 给待部署的首名医疗保留最后一个可用位置，回合末按面板身份依次部署。
        board.pending_deploy = true;
        return result;
    }
    if (!slot) {
        LogWarn << "new bench slot not found after purchase:" << card.name;
        handle_popups();
        board.pending_deploy = true;
        return result;
    }
    if (!deploy_from_bench(board, *slot, card.name, tier_hint, before)) {
        board.pending_deploy = true;
    }
    return result;
}

void StrongholdProtocolTaskPlugin::assign_opening_tiles(std::vector<OpeningOper>& opening)
{
    std::stable_sort(opening.begin(), opening.end(), [](const OpeningOper& a, const OpeningOper& b) {
        return opening_front_priority(a.name) > opening_front_priority(b.name);
    });
    for (size_t i = 0; i < opening.size(); ++i) {
        const auto role = StrongholdProtocol.get_role(opening[i].name);
        if (role != battle::Role::Unknown && role != battle::Role::Drone) {
            opening[i].tile = i == 0 && role != battle::Role::Medic ? "path0" : "rear0";
        }
    }
}

void StrongholdProtocolTaskPlugin::deploy_opening(BoardState& board, std::vector<OpeningOper>& opening)
{
    std::erase_if(opening, [&](const OpeningOper& oper) {
        const auto uncertain = board.unconfirmed_tiles.find(oper.tile);
        const auto copies = std::ranges::count(board.bench_known, oper.name, &std::pair<std::string, int>::first);
        // 若唯一的副本仍能在席位上核实，可重试上次未确认的空格。
        const auto held = board.purchases.find(oper.name);
        const bool retry = uncertain != board.unconfirmed_tiles.end() && uncertain->second == oper.name &&
                           !board.occ.contains(oper.tile) && copies == 1 && held != board.purchases.end() &&
                           held->second.unmerged + held->second.promotions == 1;
        if (oper.tile.empty() || (uncertain != board.unconfirmed_tiles.end() && !retry) ||
            board.occ.contains(oper.tile)) {
            // 预定位置已不可用，交回普通备战席重扫按当前棋盘选格。
            board.pending_deploy = true;
            return true;
        }
        if (need_exit()) {
            return false;
        }
        const auto try_slot = [&](const Point& p) {
            if (need_exit()) {
                return false;
            }
            if (retry) {
                board.unconfirmed_tiles.erase(oper.tile);
            }
            if (deploy_from_bench(board, p, oper.name, oper.tier, {}, oper.tile)) {
                return true;
            }
            if (retry) {
                board.unconfirmed_tiles.try_emplace(oper.tile, oper.name);
            }
            return false;
        };
        if (!retry && oper.bench && try_slot(*oper.bench)) {
            return true;
        }
        // 再买一人或部署上一人后，原格可能已换人。只有面板姓名一致才能使用预定的站位。
        const auto current = detect_bench_opers(ctrler()->get_image());
        return std::ranges::any_of(current, [&](const Point& p) {
            return (retry || !oper.bench || Point::distance(p, *oper.bench) >= 8) && try_slot(p);
        });
    });
    board.pending_deploy |= !opening.empty();
}

static constexpr int MaxDeploymentSlots = 8;

bool StrongholdProtocolTaskPlugin::is_opening(BoardState& board, std::optional<int> slots)
{
    board.opening_finished |= !board.occ.empty() || (slots && *slots >= 0 && *slots < MaxDeploymentSlots);
    return !board.opening_finished && slots == MaxDeploymentSlots && board.unconfirmed_tiles.empty() &&
           board.promotion_tiles.empty() && !board.unknown_displaced && board.displaced.empty();
}

// ---------------- 局内：回合准备主流程 ----------------

bool StrongholdProtocolTaskPlugin::prep_phase(BoardState& board)
{
    auto image = ctrler()->get_image();
    auto shop = read_shop(image, board);
    int round = shop.round.value_or(-1);
    if (round < 0) {
        // 回合号识别失败：按已完成回合 +1 推进（首回合保底为 1），不因失败卡死在同一回合
        round = std::max(1, board.round_done + 1);
    }
    if (round == board.round_done) {
        // 本回合已处理但仍停在准备界面：说明上次 ｢准备就绪｣ 未生效，
        // 需再点一次确认才能推进，否则死循环在同一回合
        ProcessTask(*this, { "StrongholdProtocol@ReadyClick" }).run();
        handle_popups();
        return true;
    }

    LogInfo << "round" << round << "gold" << shop.gold.value_or(-1) << "maxed" << shop.maxed;

    std::vector<OpeningOper> opening;
    auto slots = slots_left(image);
    const bool empty_field = is_opening(board, slots);
    bool shop_changed = inspect_new_bench(board, image);
    if (m_bench_shop_unconfirmed) {
        return false;
    }
    if (empty_field) {
        for (const auto& slot : board.bench_roster) {
            if (slot.name && slot.rank) {
                opening.push_back({ *slot.name, slot.rank->tier, slot.point, {} });
            }
        }
        if (opening.size() > 2) {
            assign_opening_tiles(opening);
            opening.resize(2);
        }
        if (shop_changed) {
            image = ctrler()->get_image();
            shop = read_shop(image, board);
            shop_changed = false;
        }
        int budget = shop.gold.value_or(-1);
        std::vector<int> failed_slots;
        std::vector<ShopCard> previous;
        while (opening.size() < 2 && !need_exit()) {
            const int remaining = 2 - static_cast<int>(opening.size());
            auto cards = read_shop_cards(image, previous);
            const bool reward_shop = is_reward_shop(cards);
            std::erase_if(cards, [&](const ShopCard& c) {
                return (reward_shop && c.cost != 0) ||
                       std::ranges::any_of(failed_slots, [&](int x) { return std::abs(c.x - x) < 30; });
            });
            auto pick = reward_shop ? shop_pick(cards, board, 0, -1) : opening_pick(cards, budget, remaining, board);
            if (!pick) {
                break;
            }
            previous = std::move(cards);
            std::erase_if(previous, [&](const ShopCard& c) { return reward_shop || std::abs(c.x - pick->x) < 30; });
            shop_changed = true;
            if (buy_and_deploy(board, *pick, ShopPhase::Opening, &opening) == PurchaseResult::Bought) {
                if (budget >= 0) {
                    budget = std::max(0, budget - pick->cost);
                }
                failed_slots.clear();
            }
            else {
                failed_slots.emplace_back(pick->x);
            }
            image = ctrler()->get_image();
            budget = read_gold(image).value_or(budget);
        }
        if (!opening.empty()) {
            assign_opening_tiles(opening);
            deploy_opening(board, opening);
            shop_changed = true;
            slots = slots_left(ctrler()->get_image());
        }
    }
    if (opening.empty() && std::ranges::any_of(board.bench_roster, [&](const BenchSlot& slot) {
            return slot.name && slot.rank &&
                   choose_tile(board, *slot.name, *slot.rank, {}, slots.value_or(-1), slot.level_unread);
        })) {
        board.pending_deploy = deploy_bench(board);
        shop_changed = true;
    }
    if (m_bench_shop_unconfirmed) {
        return false;
    }
    if (shop_changed) {
        image = ctrler()->get_image();
        update_bench_roster(board, detect_bench_opers(image));
        reconcile_bench_copies(board);
        shop = read_shop(image, board);
    }
    if (!shop.maxed) {
        // 关闭干员详情或升级后，整排商店会横移。连续两帧文字与位置一致后才使用按钮和费用。
        const auto settled_button = [&](bool after_confirm) -> std::optional<TextRect> {
            std::optional<TextRect> previous;
            for (int retry = 0; retry < 5 && !need_exit(); ++retry) {
                sleep(150);
                image = ctrler()->get_image();
                OCRer ocr(image);
                ocr.set_task_info("StrongholdProtocol@ShopBottomOcr");
                const auto rows = ocr.analyze();
                if (!rows || (after_confirm && rows->front().text == "confirm")) {
                    previous.reset();
                    continue;
                }
                const auto& button = rows->front();
                if (previous && previous->text == button.text && std::abs(previous->rect.x - button.rect.x) <= 4) {
                    return button;
                }
                previous = button;
            }
            return std::nullopt;
        };
        const auto level = [&](const TextRect& button) {
            const auto task = Task.get<OcrTaskInfo>("StrongholdProtocol@ShopLevelOcr");
            if (!task || task->roi.empty()) {
                return 0;
            }
            auto roi = task->roi;
            roi.x += button.rect.x + button.rect.width / 2;
            OCRer ocr(image);
            ocr.set_task_info(task);
            ocr.set_roi(roi);
            const auto rows = ocr.analyze();
            if (rows && rows->front().score >= 0.75) {
                int value = 0;
                if (utils::chars_to_number<int, true>(rows->front().text, value)) {
                    return value;
                }
            }
            return 0;
        };
        bool upgrade_pending = false;
        auto button = settled_button(false);
        shop = read_shop(image, board);
        for (int i = 0; i < 8 && button && !shop.maxed && !need_exit(); ++i) {
            if (shop.gold && shop.upgrade_cost && *shop.gold < *shop.upgrade_cost) {
                break;
            }
            const ShopInfo before = shop;
            const int old_level = level(*button);
            ProcessTask confirm(*this, { "StrongholdProtocol@UpgradeConfirmClick" });
            if (button->text == "confirm") {
                confirm.set_reusable_image(image);
            }
            else if (button->text != "upgrade" || !ctrler()->click(button->rect)) {
                break;
            }
            upgrade_pending = true;
            if (!confirm.set_task_delay(150).set_retry_times(4).run()) {
                break;
            }
            for (int retry = 0; retry < 2 && !need_exit(); ++retry) {
                button = settled_button(true);
                shop = read_shop(image, board);
                if (!button || shop.maxed) {
                    break;
                }
                if ((before.gold && shop.gold && *shop.gold < *before.gold) ||
                    (before.upgrade_cost && shop.upgrade_cost && *shop.upgrade_cost != *before.upgrade_cost) ||
                    (old_level > 0 && level(*button) > old_level)) {
                    upgrade_pending = false;
                    LogInfo << "shop upgraded, gold" << before.gold.value_or(-1) << "->" << shop.gold.value_or(-1);
                    break;
                }
            }
            if (upgrade_pending) {
                break;
            }
        }
        if (!shop.maxed && !upgrade_pending && shop.gold != 0) {
            // 剩余资金买干员（资金不跨回合，不买装备）
            for (const auto& card : read_shop_cards(image)) {
                image = ctrler()->get_image();
                const auto gold = read_gold(image);
                if (gold == 0) {
                    break;
                }
                if (!card.is_oper || card.cost < 0 || (gold && card.cost > *gold)) {
                    continue;
                }
                buy_and_deploy(board, card, ShopPhase::Early);
                if (m_bench_shop_unconfirmed) {
                    return false;
                }
            }
        }
    }

    if (shop.maxed) {
        // 满级先领赠卡，再按本局偏好购买 V/VI 阶干员。
        std::string last_signature;
        std::vector<int> skipped_slots;
        bool refreshed = false;
        int same_count = 0;
        for (int i = 0; i < 40 && !need_exit(); ++i) {
            image = ctrler()->get_image();
            const auto gold = read_gold(image);
            auto cards = read_shop_cards(image);
            std::string signature;
            for (const auto& card : cards) {
                signature += card.name + ":" + std::to_string(card.cost) + ",";
            }
            if (refreshed && signature != last_signature) {
                skipped_slots.clear();
            }
            refreshed = false;
            if (signature == last_signature) {
                if (++same_count >= 2) {
                    LogWarn << "refresh has no effect, stop refreshing";
                    break;
                }
            }
            else {
                same_count = 0;
            }
            last_signature = signature;

            if (!is_reward_shop(cards)) {
                std::erase_if(cards, [&](const ShopCard& c) {
                    return std::ranges::any_of(skipped_slots, [&](int x) { return std::abs(c.x - x) < 30; });
                });
            }
            if (const auto pick = shop_pick(cards, board, gold.value_or(99), slots_left(image).value_or(-1))) {
                LogInfo << "buy V/VI:" << pick->name << "cost" << pick->cost;
                const auto result = buy_and_deploy(board, *pick, ShopPhase::Maxed);
                if (m_bench_shop_unconfirmed) {
                    return false;
                }
                if (result == PurchaseResult::Failed) {
                    LogWarn << "buy failed, stop purchasing this round";
                    break;
                }
                if (result == PurchaseResult::Skipped) {
                    skipped_slots.emplace_back(pick->x);
                }
                last_signature.clear();
                continue;
            }
            if (gold && *gold < 4) {
                break;
            }
            ProcessTask(*this, { "StrongholdProtocol@RefreshClick" }).run();
            // 同一架上的匿名卡已在详情确认过，刷新后才能重新检查该位置。
            refreshed = true;
        }
    }

    if (board.pending_deploy) {
        deploy_opening(board, opening);
        if (opening.empty()) {
            board.pending_deploy = deploy_bench(board);
        }
    }

    if (m_bench_shop_unconfirmed) {
        return false;
    }
    board.round_done = round;
    // 等 ｢资金不足｣ 等提示消散，避免挡住准备按钮（≤1.5s）
    for (int i = 0; i < 8 && !need_exit(); ++i) {
        sleep(200);
        OCRer probe(ctrler()->get_image());
        probe.set_task_info("StrongholdProtocol@ShopNoFundsOcr");
        if (!probe.analyze()) {
            break;
        }
    }
    ProcessTask(*this, { "StrongholdProtocol@ReadyClick" }).run();
    handle_popups();
    return true;
}

void StrongholdProtocolTaskPlugin::handle_popups()
{
    const auto image = ctrler()->get_image();
    OCRer popup(image);
    popup.set_task_info("StrongholdProtocol@ConfirmReadyOcr");
    if (popup.analyze()) {
        ProcessTask(*this, Task.get("StrongholdProtocol@ConfirmReadyOcr")->next)
            .set_reusable_image(image)
            .set_retry_times(1)
            .run();
    }
}

// ---------------- 局内：部署 ----------------

static bool has_bench_control(const cv::Mat& image, const std::string& task)
{
    OCRer ocr(image);
    ocr.set_task_info(task);
    return ocr.analyze().has_value();
}

static bool is_bench_shop(const cv::Mat& image)
{
    return has_bench_control(image, "StrongholdProtocol@ShopExpanded") &&
           has_bench_control(image, "StrongholdProtocol@ReadyButton");
}

static bool same_bench_slot(const Point& a, const Point& b)
{
    const auto scan = Task.get("StrongholdProtocol@Roi@BenchScan");
    return scan && scan->special_params.size() == 3 && Point::distance(a, b) <= scan->special_params[2];
}

cv::Mat StrongholdProtocolTaskPlugin::inspect_bench(const Point& bench, const cv::Mat& shop)
{
    if (need_exit() || m_bench_shop_unconfirmed) {
        return {};
    }
    m_bench_shop_unconfirmed = true;
    if (!is_bench_shop(shop) || !ctrler()->click(bench)) {
        return {};
    }
    cv::Mat image;
    for (int i = 0; i < 6 && !need_exit(); ++i) {
        sleep(150);
        image = ctrler()->get_image();
        if (has_bench_control(image, "StrongholdProtocol@BenchDetailClose") &&
            has_bench_control(image, "StrongholdProtocol@DragTraitOcr") &&
            has_bench_control(image, "StrongholdProtocol@ReadyButton")) {
            // 棋盘已横移。仅在休整详情中点击顶部提示，商店恢复后才能使用原席位。
            if (need_exit() || !ctrler()->click(Task.get("StrongholdProtocol@BenchDetailClose")->specific_rect)) {
                return {};
            }
            const auto original = detect_bench_opers(shop);
            for (int retry = 0; retry < 6 && !need_exit(); ++retry) {
                sleep(150);
                const auto restored = ctrler()->get_image();
                m_bench_shop_unconfirmed = !is_bench_shop(restored);
                if (!m_bench_shop_unconfirmed &&
                    std::ranges::equal(original, detect_bench_opers(restored), same_bench_slot)) {
                    return image;
                }
            }
            LogWarn << __FUNCTION__ << "could not confirm the original bench positions after inspection";
            return {};
        }
    }
    // 点到空席时商店仍展开，原布局可继续使用。
    m_bench_shop_unconfirmed = image.empty() || !is_bench_shop(image);
    return {};
}

bool StrongholdProtocolTaskPlugin::swipe_from_bench(const Point& bench, const Point& target)
{
    const auto swipe = Task.get("BattleSwipeOper");
    const int duration = std::max(
        swipe->special_params.at(4),
        static_cast<int>(Point::distance(bench, target) * swipe->pre_delay / 1000.0));
    return ctrler()->swipe(
        bench,
        target,
        duration,
        SwipeExtraDirection::None,
        swipe->special_params.at(2) / 10.0,
        swipe->special_params.at(3) / 10.0);
}

static std::vector<Rect> detect_bench_badges(const cv::Mat& image)
{
    const auto scan = Task.get("StrongholdProtocol@Roi@BenchScan");
    if (!scan || scan->special_params.size() != 3) {
        return {};
    }
    return locate_tier_badges(image, scan->roi, scan->special_params[0], scan->special_params[1]);
}

static std::optional<Rect> bench_badge_at(const cv::Mat& image, const Point& bench)
{
    const auto press = task_roi("StrongholdProtocol@Roi@BenchPressOffset");
    std::optional<Rect> found;
    for (const auto& badge : detect_bench_badges(image)) {
        if (same_bench_slot(bench, { badge.x + press.x, badge.y + press.y })) {
            if (found) {
                return std::nullopt;
            }
            found = badge;
        }
    }
    return found;
}

static int selected_bench_tier(const cv::Mat& preview, const cv::Mat& rest, const Point& bench)
{
    if (const int tier = panel_tier(preview)) {
        return tier;
    }
    const auto badge = bench_badge_at(rest, bench);
    return badge ? stronghold::match_tier_badge(rest, *badge) : 0;
}

struct BenchIdentity
{
    std::string name;      // 空名字表示已辨认为装备或召唤物。
    bool verified = false; // 退场副本需要完整姓名和干员标签。
};

static std::optional<BenchIdentity> read_bench_identity(const cv::Mat& image, bool check_unreadable)
{
    if (image.empty()) {
        return std::nullopt;
    }
    const bool oper_tab = has_bench_control(image, "StrongholdProtocol@BenchOperatorTab");
    if (!oper_tab && !check_unreadable) {
        return std::nullopt;
    }
    const auto name_task = Task.get<OcrTaskInfo>("StrongholdProtocol@DragNameOcr");
    if (!name_task) {
        return std::nullopt;
    }
    OCRer ocr(image);
    ocr.set_task_info(name_task);
    const auto rows = ocr.analyze();
    if (!rows) {
        return std::nullopt;
    }
    const auto& best = rows->front();
    const auto& name = best.text;
    const auto role = StrongholdProtocol.get_role(name);
    if (role == battle::Role::Drone ||
        (!BattleData.get_all_oper_names().contains(name) && BattleData.find_first_oper(battle::Role::Drone, name))) {
        return BenchIdentity {};
    }
    if (!oper_tab || role == battle::Role::Unknown) {
        if (check_unreadable && !best.rect.empty()) {
            RegionOCRer item(image);
            item.set_task_info("StrongholdProtocol@BenchQuotedItemName");
            item.set_roi(best.rect);
            if (item.analyze()) {
                return BenchIdentity {};
            }
        }
        return std::nullopt;
    }
    const bool verified = !name_task->special_params.empty() && best.score >= name_task->special_params[0] / 100.0;
    return BenchIdentity { name, verified };
}

std::vector<Point> StrongholdProtocolTaskPlugin::ordered_bench(
    const BoardState& board,
    const cv::Mat& rest,
    const std::vector<Point>& slots)
{
    if (slots.size() < 2 || !std::ranges::any_of(board.purchases, [&](const auto& bought) {
            return may_have_bench_elite(board, bought.first);
        })) {
        return slots;
    }
    std::vector<std::pair<std::pair<int, bool>, Point>> ranked;
    for (const auto& slot : slots) {
        if (need_exit()) {
            return {};
        }
        const auto preview = inspect_bench(slot, ctrler()->get_image());
        if (m_bench_shop_unconfirmed) {
            return {};
        }
        int tier = 0;
        const auto identity = read_bench_identity(preview, false);
        bool elite = false;
        if (identity && !identity->name.empty()) {
            tier = selected_bench_tier(preview, rest, slot);
            elite = may_have_bench_elite(board, identity->name) && panel_is_elite(preview, tier).value_or(false);
        }
        ranked.push_back({ { tier, elite }, slot });
    }
    std::stable_sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    std::vector<Point> ordered;
    for (const auto& candidate : ranked) {
        ordered.emplace_back(candidate.second);
    }
    return ordered;
}

bool StrongholdProtocolTaskPlugin::inspect_new_bench(BoardState& board, const cv::Mat& image)
{
    const auto points = detect_bench_opers(image);
    update_bench_roster(board, points);
    std::vector<BenchSlot*> pending, cached;
    for (auto& slot : board.bench_roster) {
        (slot.name && (slot.name->empty() || slot.rank) ? cached : pending).push_back(&slot);
    }
    for (size_t i = 0; i < pending.size() && !need_exit(); ++i) {
        auto& slot = *pending[i];
        const auto preview = inspect_bench(slot.point, ctrler()->get_image());
        if (m_bench_shop_unconfirmed) {
            return true;
        }
        const auto identity = read_bench_identity(preview, true);
        if (!identity || (!identity->name.empty() && !identity->verified)) {
            continue;
        }
        // 新位置出现旧名单中的名字时，原位置可能已经换人，须在同一批席位里重新核对。
        if (std::ranges::any_of(cached, [&](const BenchSlot* old) { return old->name == identity->name; })) {
            for (auto* old : cached) {
                *old = { old->point, std::nullopt, std::nullopt };
                pending.emplace_back(old);
            }
            cached.clear();
        }
        slot.name = identity->name;
        if (identity->name.empty()) {
            continue;
        }
        const int tier = selected_bench_tier(preview, image, slot.point);
        if (tier > 0) {
            const auto elite = panel_is_elite(preview, tier);
            slot.rank = DeployRank { tier, elite.value_or(false) };
            slot.level_unread = !elite.has_value();
            LogInfo << "bench operator" << *slot.name << "tier" << tier << "elite" << (elite ? int(*elite) : -1) << "at"
                    << slot.point;
        }
    }
    if (!pending.empty()) {
        reconcile_bench_copies(board);
    }
    return !pending.empty();
}

void StrongholdProtocolTaskPlugin::reconcile_bench_copies(BoardState& board)
{
    std::map<std::string, PurchaseCount> observed;
    for (const auto& slot : board.bench_roster) {
        if (!slot.name || !slot.rank) {
            continue;
        }
        auto& copies = observed[*slot.name];
        ++copies.total;
        if (!slot.level_unread) {
            ++(slot.rank->elite ? copies.promotions : copies.unmerged);
        }
        // 买入和换下的副本均可能在这些席位中，只补充实际多出的待部署副本。
        const auto known = std::ranges::count(board.bench_known, *slot.name, &std::pair<std::string, int>::first);
        if (!board.unknown_displaced && copies.total > known + static_cast<int>(board.displaced.count(*slot.name))) {
            board.bench_known.emplace_back(*slot.name, slot.rank->tier);
        }
    }
    for (const auto& [key, oper] : board.occ) {
        if (!board.unconfirmed_tiles.contains(key) && !board.promotion_tiles.contains(key) &&
            !board.unread_level_tiles.contains(key)) {
            auto& copies = observed[oper.first];
            ++(board.elite_tiles.contains(key) ? copies.promotions : copies.unmerged);
        }
    }
    for (const auto& [name, seen] : observed) {
        auto& held = board.purchases[name];
        if (seen.unmerged > held.unmerged || seen.promotions > held.promotions) {
            held.unmerged = std::max(held.unmerged, seen.unmerged);
            held.promotions = std::max(held.promotions, seen.promotions);
            LogInfo << "reconciled copies" << name << "ordinary" << held.unmerged << "elite" << held.promotions;
        }
    }
}

std::optional<StrongholdProtocolTaskPlugin::DeployTile> StrongholdProtocolTaskPlugin::choose_tile(
    const BoardState& board,
    const std::string& name,
    DeployRank rank,
    const std::string& opening_tile,
    int slots,
    bool empty_only)
{
    const auto& tiles = StrongholdProtocol.get_tiles();
    const auto role = StrongholdProtocol.get_role(name);
    if (slots < 0 || slots > MaxDeploymentSlots || role == battle::Role::Unknown || role == battle::Role::Drone) {
        return std::nullopt;
    }
    const bool medic = role == battle::Role::Medic;
    const auto field = medic ? field_medic_tier(board) : std::nullopt;
    const int tier = rank.tier;
    const bool field_elite = medic && std::ranges::any_of(board.elite_tiles, [&](const auto& key) {
                                 return is_medic(board.occ.at(key).first);
                             });
    if (field && (!*field || rank <= DeployRank { *field, field_elite })) {
        return std::nullopt;
    }
    std::optional<DeployTile> promotion_fallback;
    std::optional<DeployTile> medic_swap;
    DeployRank medic_swap_rank { 5, false };
    const bool melee = opening_front_priority(name) > 0;
    const auto pick_group = [&](const std::string& group,
                                const std::vector<Point>& points) -> std::optional<DeployTile> {
        std::optional<DeployTile> promotion;
        std::optional<DeployTile> swap;
        auto weakest = rank;
        for (size_t i = 0; i < points.size(); ++i) {
            DeployTile tile { group + std::to_string(i), points[i] };
            if (board.unconfirmed_tiles.contains(tile.key) || (!opening_tile.empty() && tile.key != opening_tile)) {
                continue;
            }
            if (tile.key == "path0" && !melee && (opening_tile.empty() || role == battle::Role::Medic)) {
                continue;
            }
            const auto occupant = board.occ.find(tile.key);
            if (board.promotion_tiles.contains(tile.key)) {
                // 合成素材可能还占着该格。优先用本组已知空格，仅在有名额且能辨认原干员时试放。
                if (!promotion && slots > 0 && !field && !(empty_only && board.unknown_displaced) &&
                    occupant != board.occ.end()) {
                    const auto& [old_name, old_tier] = occupant->second;
                    // 配对格恰有一个空位。试放换下精锐时，名额会确认另一空格，供精锐补位。
                    const auto old_role = StrongholdProtocol.get_role(old_name);
                    if (old_role != battle::Role::Unknown && old_role != battle::Role::Drone &&
                        old_role != battle::Role::Medic && old_tier > 0 &&
                        (board.promotion_partners.contains(tile.key) || rank >= DeployRank { old_tier, false })) {
                        promotion = tile;
                    }
                }
                continue;
            }
            if (occupant == board.occ.end()) {
                if (!field && slots > 0) {
                    return tile;
                }
                continue;
            }
            if (empty_only || (!opening_tile.empty() && !medic)) {
                continue;
            }
            const auto& [old_name, old_tier] = occupant->second;
            const auto old_role = StrongholdProtocol.get_role(old_name);
            const DeployRank old_rank { old_tier, board.elite_tiles.contains(tile.key) };
            if (old_tier <= 0 || old_role == battle::Role::Unknown || old_role == battle::Role::Drone ||
                ((rank.elite || old_rank.elite) && rank < old_rank) ||
                (old_tier >= 5 && ((!rank.elite && !old_rank.elite) || rank <= old_rank))) {
                continue;
            }
            if (old_role == battle::Role::Medic) {
                if (medic && field) {
                    return tile;
                }
                continue;
            }
            const bool old_melee = opening_front_priority(old_name) > 0;
            if (medic) {
                // 首名医疗优先用空格。所有兼容格都满时，才换下其中等阶最低的干员。
                if (!field && (!old_melee || group != "path") && old_rank < medic_swap_rank) {
                    medic_swap_rank = old_rank;
                    medic_swap = tile;
                }
                continue;
            }
            if (group == "path" && melee && (!old_melee || tier >= 5)) {
                // 堵路格按接敌顺序补强。新近战接替低阶远程，高阶近战也接替低阶近战。
                return tile;
            }
            const bool keep_melee = group == "path" || (group == "rear" && tier < 5);
            if ((!melee && old_melee && keep_melee) || (melee && !old_melee && group == "safe")) {
                continue;
            }
            if (old_rank < weakest) {
                weakest = old_rank;
                swap = tile;
            }
        }
        if (empty_only) {
            // 先看其余职业兼容的已知空格，避免为补位换下仍留在合成格中的干员。
            if (!promotion_fallback) {
                promotion_fallback = promotion;
            }
            return std::nullopt;
        }
        return promotion ? promotion : swap;
    };
    if (auto tile = melee ? pick_group("path", tiles.path) : pick_group("safe", tiles.safe)) {
        return tile;
    }
    if (auto tile = pick_group("rear", tiles.rear)) {
        return tile;
    }
    if (auto tile = melee ? pick_group("safe", tiles.safe) : pick_group("path", tiles.path)) {
        return tile;
    }
    return empty_only ? promotion_fallback : medic_swap;
}

StrongholdProtocolTaskPlugin::Placement
    StrongholdProtocolTaskPlugin::drag_from_bench(const Point& bench, const Point& tile)
{
    if (!swipe_from_bench(bench, tile)) {
        return Placement::Unconfirmed;
    }
    for (int i = 0; i < 10 && !need_exit(); ++i) {
        sleep(200);
        if (has_facing_cancel(ctrler()->get_image(), tile)) {
            if (set_facing(tile)) {
                m_last_tile.reset();
                return Placement::Confirmed;
            }
            m_last_tile = tile;
            return Placement::FacingPending;
        }
    }
    save_img(utils::path("debug") / utils::path("stronghold"), false);
    return Placement::Unconfirmed;
}

std::optional<StrongholdProtocolTaskPlugin::BoardDrop> StrongholdProtocolTaskPlugin::place_on_board(
    BoardState& board,
    const DeployTile& tile,
    const Point& bench,
    const std::string& name,
    DeployRank rank,
    int slots)
{
    LogInfo << "deploy target" << tile.key << tile.point.x << "," << tile.point.y;
    for (auto& slot : board.bench_roster) {
        if (same_bench_slot(slot.point, bench)) {
            slot = { slot.point, std::nullopt, std::nullopt };
        }
    }
    const auto placement = drag_from_bench(bench, tile.point);
    auto after = ctrler()->get_image();
    auto remaining = slots_left(after);
    const auto valid_capacity = [&] {
        return remaining && *remaining >= 0 && (*remaining == slots || *remaining == slots - 1);
    };
    if (placement == Placement::Confirmed && !valid_capacity()) {
        const auto counter = Task.get("StrongholdProtocol@ShopSlotsOcr");
        if (counter && counter->special_params.size() == 1) {
            for (int retry = 0; retry < counter->special_params.front() && !valid_capacity() && !need_exit(); ++retry) {
                sleep(counter->post_delay);
                after = ctrler()->get_image();
                remaining = slots_left(after);
            }
        }
    }
    if (placement == Placement::Unconfirmed && slots > 0 && remaining == slots &&
        std::ssize(board.occ) + slots == MaxDeploymentSlots && !board.occ.contains(tile.key) &&
        board.unconfirmed_tiles.empty() && board.promotion_tiles.empty() && !board.unknown_displaced && !need_exit() &&
        is_bench_shop(after) && bench_badge_at(after, bench)) {
        // 已知空格没有消耗名额，原席仍是同一干员时，回合内可重新尝试这个格子。
        const auto original = read_bench_identity(inspect_bench(bench, after), false);
        if (original && original->verified && original->name == name) {
            board.pending_deploy = true;
            LogInfo << "operator still on bench after swipe" << name << tile.key;
            return std::nullopt;
        }
    }
    if (placement != Placement::Confirmed || !valid_capacity()) {
        // 结果不明时，原占位和本次拿起的干员都可能仍在该格。后续选格须避开它。
        board.unconfirmed_tiles[tile.key] = name;
        board.unknown_displaced = true;
        board.pending_deploy = true;
        LogWarn << "operator facing or capacity unconfirmed" << name << tile.key << slots << remaining.value_or(-1);
        return std::nullopt;
    }
    resolve_promotion_tile(board, tile.key, *remaining == slots);
    BoardDrop result { std::move(after), *remaining, std::nullopt };
    if (*remaining == slots) {
        board.pending_deploy = true;
        if (const auto old = board.occ.find(tile.key);
            old != board.occ.end() && !board.unread_level_tiles.contains(tile.key)) {
            const DeployRank old_rank { old->second.second, board.elite_tiles.contains(tile.key) };
            if (old->second.first != name || old_rank != rank) {
                result.displaced.emplace(old->second.first, old_rank);
            }
            board.displaced.emplace(old->second.first, old_rank);
        }
        else {
            board.unknown_displaced = true;
            LogWarn << "unexpected swap at" << tile.key << "with unidentified occupant";
        }
    }
    board.opening_finished = true;
    board.unread_level_tiles.erase(tile.key);
    board.occ[tile.key] = { name, rank.tier };
    if (rank.elite) {
        board.elite_tiles.emplace(tile.key);
    }
    else {
        board.elite_tiles.erase(tile.key);
    }
    LogInfo << "deployed" << name << "tier" << rank.tier << "elite" << rank.elite << "to" << tile.key << tile.point.x
            << "," << tile.point.y << "slots" << slots << "->" << *remaining;
    return result;
}

bool StrongholdProtocolTaskPlugin::can_sell_replaced_medic(
    const BoardState& board,
    const std::string& name,
    DeployRank rank)
{
    if (!is_medic(name) || board.unknown_displaced || board.displaced.count(name) != 1 ||
        std::ranges::any_of(board.bench_known, [&](const auto& copy) { return copy.first == name; })) {
        return false;
    }
    // 换人时已记录退场副本的等阶。重扫时仍须确认场上医疗在位，且退场者不会反过来成为升级人选。
    return std::ranges::any_of(board.occ, [&](const auto& entry) {
        const auto& [key, oper] = entry;
        return is_medic(oper.first) && !board.promotion_tiles.contains(key) && !board.unconfirmed_tiles.contains(key) &&
               oper.second > 0 && DeployRank { oper.second, board.elite_tiles.contains(key) } >= rank;
    });
}

bool StrongholdProtocolTaskPlugin::can_sell_displaced_non_medic(
    const BoardState& board,
    const std::string& name,
    DeployRank rank)
{
    return !is_medic(name) && rank.tier > 0 && rank.tier < 5 && !rank.elite && StrongholdProtocol.priority(name) == 0 &&
           BattleData.get_all_oper_names().contains(name) && !BattleData.find_first_oper(battle::Role::Drone, name) &&
           !board.unknown_displaced && board.displaced.count(name) == 1 &&
           std::ranges::count(board.bench_roster, name, &BenchSlot::name) <= 1 &&
           std::ranges::none_of(
               board.bench_known,
               [&](const auto& copy) { return copy.first == name; }) &&
           std::ranges::none_of(board.occ, [&](const auto& entry) { return entry.second.first == name; });
}

bool StrongholdProtocolTaskPlugin::sell_displaced(
    BoardState& board,
    DisplacedOpers::iterator copy,
    const Point& bench,
    int gold,
    const cv::Mat& preview)
{
    const auto [name, rank] = *copy;
    const bool replaced_medic = can_sell_replaced_medic(board, name, rank);
    const int tier = panel_tier(preview);
    if ((tier && rank.tier && tier != rank.tier) || (!replaced_medic && (!tier || !rank.tier))) {
        return false;
    }
    if (rank.elite || may_have_bench_elite(board, name)) {
        const auto elite = panel_is_elite(preview, tier);
        if ((elite && *elite != rank.elite) || (!elite && !replaced_medic)) {
            return false;
        }
    }
    const Rect sell = task_roi("StrongholdProtocol@Roi@ShopSell");
    // 售出提示只在拖动中出现。起拖前核对商店位置、原席位和资金。
    const auto shop = ctrler()->get_image();
    const auto badge = bench_badge_at(shop, bench);
    if (!badge) {
        return false;
    }
    const int bench_tier = stronghold::match_tier_badge(shop, *badge);
    if (bench_tier && ((tier && bench_tier != tier) || (rank.tier && bench_tier != rank.tier))) {
        return false;
    }
    if (replaced_medic && !rank.tier &&
        !can_sell_replaced_medic(board, name, { tier ? tier : bench_tier, rank.elite })) {
        return false;
    }
    OCRer drawer(shop);
    drawer.set_task_info("StrongholdProtocol@ShopExpanded");
    const auto buttons = drawer.analyze();
    if (!buttons || !sell.include(buttons->front().rect) ||
        !has_bench_control(shop, "StrongholdProtocol@ReadyButton") ||
        read_number(shop, "StrongholdProtocol@ShopGoldWideOcr") != gold || need_exit() ||
        !swipe_from_bench(bench, { sell.x + sell.width / 2, sell.y + sell.height / 2 })) {
        return false;
    }
    // 发出手势就核销退场记录。原席是否还在由下次方牌检测确认，其他席位的身份仍然有效。
    board.displaced.erase(copy);
    for (auto& slot : board.bench_roster) {
        if (same_bench_slot(slot.point, bench)) {
            slot = { slot.point, std::nullopt, std::nullopt };
        }
    }
    const auto held = board.purchases.find(name);
    if (!rank.elite && held != board.purchases.end() && held->second.unmerged > 0) {
        // 余额漏读时也不能再拿这张卡凑第三份。仍在原席的普通副本可从实物数量恢复。
        --held->second.unmerged;
    }
    sleep(1000);
    const auto after = read_gold(ctrler()->get_image());
    if (!after || *after <= gold) {
        LogWarn << "sale unconfirmed:" << name;
        return true;
    }
    if (rank.elite && held != board.purchases.end() && held->second.promotions > 0) {
        --held->second.promotions;
    }
    LogInfo << "sold swapped-out operator" << name << "gold" << gold << "->" << *after;
    return true;
}

void StrongholdProtocolTaskPlugin::handle_displaced(BoardState& board, BoardDrop drop, cv::Mat before, Point bench)
{
    // 补位只试空格或合成待核格。每次试放都会核实一个格位，因此不会在已知占位之间反复交换。
    while (drop.displaced && !need_exit()) {
        const auto [name, rank] = *drop.displaced;
        const bool replaced_medic = can_sell_replaced_medic(board, name, rank);
        if ((!replaced_medic && rank.tier <= 0) || !BattleData.get_all_oper_names().contains(name) ||
            BattleData.find_first_oper(battle::Role::Drone, name)) {
            return;
        }
        const auto empty = replaced_medic ? std::nullopt : choose_tile(board, name, rank, {}, drop.slots, true);
        const bool may_sell = replaced_medic || can_sell_displaced_non_medic(board, name, rank);
        if (!empty && !may_sell) {
            return;
        }
        const auto gold = empty ? std::optional<int> {} : read_gold(drop.after);
        if (!empty && !gold) {
            return;
        }
        const auto [first, last] = board.displaced.equal_range(name);
        const auto copy = std::find_if(first, last, [&](const auto& entry) { return entry.second == rank; });
        if (copy == last) {
            return;
        }
        const auto changed = new_bench_slot(before, drop.after);
        std::vector<Point> candidates { changed.value_or(bench) };
        if (changed && !same_bench_slot(*changed, bench)) {
            candidates.emplace_back(bench);
        }
        bool placed = false;
        for (const auto& candidate : candidates) {
            if (need_exit()) {
                return;
            }
            const auto preview = inspect_bench(candidate, ctrler()->get_image());
            if (m_bench_shop_unconfirmed) {
                return;
            }
            const auto identity = read_bench_identity(preview, false);
            if (!identity || !identity->verified || identity->name != name || need_exit()) {
                continue;
            }
            if (!empty) {
                if (sell_displaced(board, copy, candidate, *gold, preview)) {
                    return;
                }
                continue;
            }
            const int tier = selected_bench_tier(preview, drop.after, candidate);
            const auto elite = rank.elite || may_have_bench_elite(board, name) ? panel_is_elite(preview, tier) : false;
            if (tier != rank.tier || elite != rank.elite || need_exit()) {
                LogInfo << "unverified displaced operator on bench:" << name << candidate;
                continue;
            }
            auto next = place_on_board(board, *empty, candidate, name, rank, drop.slots);
            if (!next) {
                return;
            }
            board.displaced.erase(copy);
            before = std::move(drop.after);
            drop = std::move(*next);
            bench = candidate;
            placed = true;
            break;
        }
        if (!placed) {
            return;
        }
    }
}

std::optional<Point> StrongholdProtocolTaskPlugin::deploy_from_bench(
    BoardState& board,
    const Point& bench,
    const std::string& name_hint,
    int tier_hint,
    const cv::Mat& before_purchase,
    const std::string& opening_tile,
    std::optional<std::string>* observed,
    bool medic_only)
{
    if (m_bench_shop_unconfirmed || need_exit()) {
        if (observed) {
            observed->reset();
        }
        return std::nullopt;
    }
    const auto before_deploy = ctrler()->get_image();
    const auto slots0 = slots_left(before_deploy);
    const auto& tiles = StrongholdProtocol.get_tiles();
    if (slots0 == 0 && board.promotion_tiles.empty() && board.displaced.empty() && board.elite_tiles.empty() &&
        board.occ.size() == tiles.path.size() + tiles.safe.size() + tiles.rear.size() &&
        std::ranges::all_of(board.occ, [](const auto& entry) { return entry.second.second >= 5; }) &&
        std::ranges::none_of(board.purchases, [](const auto& bought) { return bought.second.promotions > 0; })) {
        return std::nullopt;
    }
    const auto preview = inspect_bench(bench, before_deploy);

    const auto identity = read_bench_identity(preview, observed != nullptr);
    for (auto& slot : board.bench_roster) {
        if (same_bench_slot(slot.point, bench) && identity && slot.name != identity->name) {
            slot = { slot.point, std::nullopt, std::nullopt };
        }
    }
    if (observed) {
        *observed = identity && (identity->verified || identity->name.empty()) ? std::optional { identity->name }
                                                                               : std::nullopt;
    }
    if (!identity || identity->name.empty() ||
        (name_hint.empty() ? !identity->verified : identity->name != name_hint)) {
        LogInfo << "bench operator not confirmed, expected" << name_hint << "read" << (identity ? identity->name : "");
        return std::nullopt;
    }
    const auto& name = identity->name;
    const auto [first, last] = board.displaced.equal_range(name);
    if (!medic_only && name_hint.empty() && first != last && can_sell_replaced_medic(board, name, first->second)) {
        if (identity->verified) {
            if (const auto gold = read_gold(before_deploy)) {
                sell_displaced(board, first, bench, *gold, preview);
            }
        }
        return std::nullopt;
    }
    const int observed_tier = selected_bench_tier(preview, before_deploy, bench);
    auto known = std::ranges::find(board.bench_known, name, &std::pair<std::string, int>::first);
    auto displaced = last;
    if (known != board.bench_known.end()) {
        // 同名方牌换位后无法按购买顺序区分。清除互相矛盾的旧等阶，当前点选的干员单独核验。
        const bool ambiguous = std::ranges::any_of(board.bench_known, [&](const auto& copy) {
            return copy.first == name && copy.second != known->second;
        });
        if (ambiguous) {
            for (auto& copy : board.bench_known) {
                if (copy.first == name) {
                    copy.second = 0;
                }
            }
        }
        tier_hint = known->second;
    }
    const int tier = observed_tier > 0 ? observed_tier : tier_hint;
    const auto cached = std::ranges::find_if(board.bench_roster, [&](const BenchSlot& slot) {
        return slot.name == name && slot.rank && slot.rank->tier == tier && same_bench_slot(slot.point, bench);
    });
    std::optional<bool> elite;
    if (cached != board.bench_roster.end() && !cached->level_unread) {
        elite = cached->rank->elite;
    }
    else if (
        !may_have_bench_elite(board, name) &&
        (!before_purchase.empty() ||
         (!board.unknown_displaced && known == board.bench_known.end() && board.displaced.count(name) == 1 &&
          !first->second.elite && first->second.tier == observed_tier))) {
        elite = false;
    }
    else {
        elite = panel_is_elite(preview, tier);
    }
    if (!elite && (first != last || board.unknown_displaced)) {
        if (observed) {
            observed->reset();
        }
        return std::nullopt;
    }
    const DeployRank rank { tier, elite.value_or(false) };
    const bool unknown_bench = board.unknown_displaced && name_hint.empty();
    if (unknown_bench || (name_hint.empty() && first != last)) {
        displaced = std::find_if(first, last, [&](const auto& copy) { return copy.second == rank; });
        if (!identity->verified || observed_tier <= 0 ||
            (displaced == last && known != board.bench_known.end() && known->second > 0 &&
             known->second != observed_tier) ||
            (first != last && displaced == last && known == board.bench_known.end())) {
            if (observed) {
                observed->reset();
            }
            return std::nullopt;
        }
    }
    const bool from_displacement = displaced != last;
    if (medic_only && !is_medic(name) && !from_displacement) {
        return std::nullopt;
    }
    const auto target =
        choose_tile(board, name, rank, opening_tile, slots0.value_or(-1), from_displacement || unknown_bench || !elite);
    if (!target) {
        // 医疗检查也可能点到待售的退场者。名额已读清且没有可补的格位时，用本次面板核验出售。
        if (from_displacement && slots0 && *slots0 <= MaxDeploymentSlots &&
            can_sell_displaced_non_medic(board, name, rank)) {
            if (const auto gold = read_gold(before_deploy)) {
                sell_displaced(board, displaced, bench, *gold, preview);
            }
            return std::nullopt;
        }
        if (is_medic(name) && !from_displacement) {
            if (known == board.bench_known.end()) {
                board.bench_known.emplace_back(name, tier);
            }
            else {
                known->second = tier;
            }
        }
        LogInfo << "no available tile for" << name;
        return std::nullopt;
    }
    if (need_exit() || (medic_only && !is_medic(name))) {
        return std::nullopt;
    }
    auto result = place_on_board(board, *target, bench, name, rank, *slots0);
    if (!result) {
        return std::nullopt;
    }
    if (!elite) {
        board.unread_level_tiles.emplace(target->key);
    }
    if (from_displacement) {
        board.displaced.erase(displaced);
    }
    if (!from_displacement && known != board.bench_known.end()) {
        board.bench_known.erase(known);
    }
    handle_displaced(board, std::move(*result), before_purchase.empty() ? before_deploy : before_purchase, bench);
    return target->point;
}

void StrongholdProtocolTaskPlugin::update_bench_roster(BoardState& board, const std::vector<Point>& slots)
{
    // 已知席位无故消失时，其余同坐标的方牌也可能来自重新排列的干员。
    const bool shifted = std::ranges::any_of(board.bench_roster, [&](const BenchSlot& old) {
        return old.name && std::ranges::none_of(slots, [&](Point p) { return same_bench_slot(old.point, p); });
    });
    std::vector<BenchSlot> current;
    current.reserve(slots.size());
    for (const auto& point : slots) {
        const auto old = std::ranges::find_if(board.bench_roster, [&](const BenchSlot& slot) {
            return same_bench_slot(slot.point, point);
        });
        if (!shifted && old != board.bench_roster.end()) {
            current.push_back({ point, old->name, old->rank, old->level_unread });
        }
        else {
            current.push_back({ point, std::nullopt, std::nullopt });
        }
    }
    board.bench_roster = std::move(current);
}

bool StrongholdProtocolTaskPlugin::deploy_bench_medic(BoardState& board, const cv::Mat& image)
{
    const auto field = field_medic_tier(board);
    const bool known_medic =
        std::ranges::any_of(board.bench_known, [](const auto& copy) { return is_medic(copy.first); });
    if (field && !known_medic && board.promotion_tiles.empty()) {
        return false;
    }
    const auto slots = detect_bench_opers(image);
    const bool pending_elite_medic = std::ranges::any_of(board.purchases, [&](const auto& bought) {
        return bought.second.promotions > 0 && is_medic(bought.first) && may_have_bench_elite(board, bought.first);
    });
    update_bench_roster(board, slots);
    auto& roster = board.bench_roster;
    std::vector<Point> candidates;
    for (const auto& slot : roster) {
        const auto& name = slot.name;
        if (!name ||
            (is_medic(*name) && (!field || !board.promotion_tiles.empty() || may_have_bench_elite(board, *name) ||
                                 std::ranges::any_of(board.bench_known, [&](const auto& copy) {
                                     return copy.first == *name && improves_field_medic(field, copy.second);
                                 })))) {
            candidates.push_back(slot.point);
        }
    }
    if (pending_elite_medic) {
        candidates = ordered_bench(board, image, candidates);
    }
    bool inspected = false;
    for (const auto& slot : candidates) {
        if (need_exit() || m_bench_shop_unconfirmed) {
            return inspected;
        }
        auto& name = std::ranges::find(roster, slot, &BenchSlot::point)->name;
        inspected = true;
        if (deploy_from_bench(board, slot, {}, 0, {}, {}, &name, true)) {
            return true;
        }
    }
    if (m_bench_shop_unconfirmed) {
        return inspected;
    }
    // 名单完整且方牌位置稳定时，才清除已离开备战席的医疗记录。
    if (known_medic && std::ranges::all_of(roster, [](const BenchSlot& slot) { return slot.name.has_value(); })) {
        const auto missing = [&](const auto& copy) {
            return is_medic(copy.first) &&
                   std::ranges::none_of(roster, [&](const BenchSlot& slot) { return slot.name == copy.first; });
        };
        if (std::ranges::any_of(board.bench_known, missing)) {
            sleep(200);
            if (detect_bench_opers(ctrler()->get_image()) == slots) {
                std::erase_if(board.bench_known, missing);
            }
        }
    }
    return inspected;
}

bool StrongholdProtocolTaskPlugin::deploy_bench(BoardState& board)
{
    deploy_bench_medic(board, ctrler()->get_image());
    if (m_bench_shop_unconfirmed) {
        return true;
    }
    // 备战席小人按头顶等阶方牌检测定位（台面布局随人数变化，固定扫点不可靠）：
    // 每轮重新检测当前剩余人，逐个尝试部署，部署成功小人消失故循环重扫
    for (int round = 0; round < 10 && !need_exit(); ++round) {
        const auto rest = ctrler()->get_image();
        const auto opers = detect_bench_opers(rest);
        update_bench_roster(board, opers);
        if (opers.empty()) {
            if (board.unknown_displaced || !board.displaced.empty()) {
                sleep(200);
                if (!detect_bench_opers(ctrler()->get_image()).empty()) {
                    return true;
                }
                board.displaced.clear();
                board.unknown_displaced = false;
            }
            return false;
        }
        auto unseen_displaced = board.displaced;
        auto unseen = board.bench_known;
        std::erase_if(unseen, [](const auto& copy) {
            const auto role = StrongholdProtocol.get_role(copy.first);
            return role == battle::Role::Medic || role == battle::Role::Drone || role == battle::Role::Unknown;
        });
        bool unreadable = false;
        bool bench_changed = false;
        std::vector<Point> candidates;
        auto& roster = board.bench_roster;
        const auto slots = slots_left(rest);
        for (const auto& slot : roster) {
            const auto& name = slot.name;
            if (name && (name->empty() ||
                         (slot.rank && !board.unknown_displaced && !board.displaced.contains(*name) &&
                          !choose_tile(board, *name, *slot.rank, {}, slots.value_or(-1), slot.level_unread)))) {
                if (auto copy = std::ranges::find(unseen, *name, &std::pair<std::string, int>::first);
                    copy != unseen.end()) {
                    unseen.erase(copy);
                }
                continue;
            }
            candidates.emplace_back(slot.point);
        }
        if (!board.unknown_displaced && !board.displaced.empty()) {
            // 只跳过名单中已核验的唯一退场副本。每次按当前名额、棋盘和出售条件重新判断。
            for (const auto& slot : roster) {
                const auto& pt = slot.point;
                const auto& name = slot.name;
                if (!slots || *slots > MaxDeploymentSlots || !name || is_medic(*name) ||
                    board.displaced.count(*name) != 1 || std::ranges::count(roster, name, &BenchSlot::name) != 1 ||
                    std::ranges::any_of(board.bench_known, [&](const auto& copy) { return copy.first == *name; })) {
                    continue;
                }
                const auto& rank = board.displaced.find(*name)->second;
                if (!can_sell_displaced_non_medic(board, *name, rank) &&
                    !choose_tile(board, *name, rank, {}, *slots, true)) {
                    std::erase(candidates, pt);
                    unseen_displaced.erase(*name);
                }
            }
        }
        for (const auto& pt : ordered_bench(board, rest, candidates)) {
            std::optional<std::string> name = std::string {}; // 满员且全为高阶时会在点选前返回。
            auto* observed = unseen.empty() && unseen_displaced.empty() ? nullptr : &name;
            const auto displaced_before = board.displaced.size();
            if (deploy_from_bench(board, pt, {}, 0, {}, {}, observed)) {
                bench_changed = true;
                break;
            }
            if (board.displaced.size() < displaced_before) {
                if (name) {
                    unseen_displaced.erase(*name);
                }
                continue;
            }
            if (observed && !board.displaced.empty()) {
                std::ranges::find(roster, pt, &BenchSlot::point)->name = name;
            }
            if (!name) {
                unreadable = true;
            }
            else if (const auto copy = unseen_displaced.find(*name); copy != unseen_displaced.end()) {
                unseen_displaced.erase(copy);
            }
            else if (
                const auto it = std::ranges::find(unseen, *name, &std::pair<std::string, int>::first);
                it != unseen.end()) {
                unseen.erase(it);
            }
        }
        if (m_bench_shop_unconfirmed) {
            return true;
        }
        if (!bench_changed) {
            // 同名份数分别核销，已核对但无格可放的干员不需要因旁边的方牌漏读而重扫。
            if (!unreadable && !unseen_displaced.empty() && detect_bench_opers(ctrler()->get_image()) == opers) {
                for (const auto& [missing, unused] : unseen_displaced) {
                    // 仍看得到同名方牌时，不能只凭名字核销其中一个等阶。
                    if (board.displaced.count(missing) == unseen_displaced.count(missing)) {
                        board.displaced.erase(missing);
                    }
                }
            }
            return board.unknown_displaced || !board.displaced.empty() || (unreadable && !unseen.empty());
        }
    }
    return board.unknown_displaced || !board.displaced.empty() || !board.bench_known.empty();
}

bool StrongholdProtocolTaskPlugin::set_facing(const Point& tile)
{
    const auto swipe = Task.get("BattleSwipeOper");
    const auto [width, height] = ctrler()->get_scale_size();
    const int distance = std::min(swipe->special_params.at(0) * height / 720, width - 1 - tile.x);
    if (distance <= 0 || !ctrler()->swipe(tile, tile + Point::right() * distance, swipe->post_delay)) {
        return false;
    }
    for (int i = 0; i < 10 && !need_exit(); ++i) {
        sleep(200);
        if (!has_facing_cancel(ctrler()->get_image(), tile)) {
            return true;
        }
    }
    LogWarn << __FUNCTION__ << "facing selection still open";
    return false;
}

std::optional<int> StrongholdProtocolTaskPlugin::slots_left(const cv::Mat& image) const
{
    auto value = read_number(image, "StrongholdProtocol@ShopSlotsOcr");
    // 最多 8，只取个位
    if (value) {
        return *value % 10;
    }
    return std::nullopt;
}

std::optional<Point> StrongholdProtocolTaskPlugin::new_bench_slot(const cv::Mat& before, const cv::Mat& after) const
{
    const cv::Rect scan = make_rect<cv::Rect>(task_roi("StrongholdProtocol@Roi@BenchScan")) &
                          cv::Rect(0, 0, std::min(before.cols, after.cols), std::min(before.rows, after.rows));
    const auto diff = Task.get("StrongholdProtocol@BenchDiff");
    if (scan.empty() || !diff || diff->special_params.size() != 3) {
        return std::nullopt;
    }
    const auto& params = diff->special_params;
    const int min_change = params[0], kernel = params[1], min_side = params[2];
    if (min_change <= 0 || min_change > 255 || kernel <= 0 || kernel > scan.height || min_side <= 0 ||
        min_side > scan.height) {
        return std::nullopt;
    }
    // 深色方牌出现时背景变暗，离开时背景变亮。
    cv::Mat before_gray, after_gray, mask;
    cv::cvtColor(before(scan), before_gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(after(scan), after_gray, cv::COLOR_BGR2GRAY);
    cv::subtract(before_gray, after_gray, mask);
    cv::threshold(mask, mask, min_change, 255, cv::THRESH_BINARY);
    // 等阶数字的笔画会在方牌内留下差分空洞。
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, { kernel, kernel }));

    cv::Rect square;
    std::vector<int> previous(mask.cols + 1), current(mask.cols + 1);
    for (int y = 0; y < mask.rows; ++y) {
        const auto* pixels = mask.ptr<uchar>(y);
        for (int x = 0; x < mask.cols; ++x) {
            // 以当前像素为右下角的正方形能扩展多少，取决于左、上、左上三个相邻正方形的最短边。
            const int side = pixels[x] ? 1 + std::min({ current[x], previous[x + 1], previous[x] }) : 0;
            current[x + 1] = side;
            if (side > square.width) {
                square = { x + 1 - side, y + 1 - side, side, side };
            }
        }
        previous.swap(current);
    }
    if (square.width >= min_side) {
        // 差分方块只覆盖变暗的牌面，起拖点以完整方牌的左上角为准。
        const Point center { scan.x + square.x + square.width / 2, scan.y + square.y + square.height / 2 };
        const auto badges = detect_bench_badges(after);
        const auto badge =
            std::ranges::find_if(badges, [&](const Rect& candidate) { return candidate.include(center); });
        if (badge != badges.end()) {
            const auto press = task_roi("StrongholdProtocol@Roi@BenchPressOffset");
            const Point pt { badge->x + press.x, badge->y + press.y };
            LogTrace << "new bench slot" << *badge << "press" << pt;
            return pt;
        }
    }

    cv::Mat diff_vis;
    cv::cvtColor(mask, diff_vis, cv::COLOR_GRAY2BGR);
    if (!square.empty()) {
        cv::rectangle(diff_vis, square, { 0, 255, 255 });
    }
    cv::Mat trip;
    cv::vconcat(std::vector<cv::Mat> { before(scan), after(scan), diff_vis }, trip);
    utils::save_debug_image(trip, utils::path("debug/stronghold"), true, "bench diff miss", "bench_diff_miss");
    return std::nullopt;
}

std::vector<Point> StrongholdProtocolTaskPlugin::detect_bench_opers(const cv::Mat& image)
{
    const Rect press = task_roi("StrongholdProtocol@Roi@BenchPressOffset");
    std::vector<Point> opers;
    for (const auto& badge : detect_bench_badges(image)) {
        const Point pt { badge.x + press.x, badge.y + press.y };
        LogTrace << "bench tile" << badge.x << badge.y << badge.width << badge.height << "press" << pt.x << pt.y;
        opers.emplace_back(pt);
    }
    // 按 x 从右往左排（小人从台面右端起占）
    std::ranges::sort(opers, [](const Point& a, const Point& b) { return a.x > b.x; });
    return opers;
}

// ---------------- 局后：结算 ----------------

bool StrongholdProtocolTaskPlugin::handle_settlement(BoardState& board)
{
    LogTraceFunction;

    // 点 ｢返回主页｣ 后页面没离开时会再次进入本函数，同一次结算只计一局
    if (!m_settlement_counted) {
        cv::Mat image;
        std::optional<bool> outcome;
        for (int i = 0; i < 5 && !need_exit(); ++i) {
            if (i > 0) {
                sleep(250);
            }
            image = ctrler()->get_image();
            OCRer result(image);
            result.set_task_info("StrongholdProtocol@SettlementResultOcr");
            if (auto rows = result.analyze(); rows && !rows->empty()) {
                const auto& title = rows->front().text;
                if (title == "cleared" || title == "interrupted") {
                    outcome = title == "cleared";
                    break;
                }
            }
        }
        if (!outcome.has_value()) {
            if (!need_exit()) {
                utils::save_debug_image(
                    image,
                    utils::path("debug/stronghold"),
                    true,
                    "unreadable settlement",
                    "settlement");
                LogError << __FUNCTION__ << "settlement title unreadable";
            }
            return false;
        }
        const bool cleared = *outcome;
        m_settlement_counted = true;
        ++m_attempted;
        if (cleared) {
            ++m_cleared;
        }

        LogInfo << "settlement:" << (cleared ? "cleared" : "failed") << ", progress" << m_cleared << "/" << m_runs
                << ", attempted" << m_attempted << ", strategy" << m_current_strategy;
        json::value info = basic_info_with_what("StrongholdSettlement");
        info["details"]["cleared"] = cleared;
        info["details"]["progress"] = m_cleared;
        info["details"]["total"] = m_runs;
        info["details"]["attempted"] = m_attempted;
        info["details"]["strategy"] = m_current_strategy;
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    click_template("StrongholdProtocol@ReturnHome");
    // 等结算页退出回到主界面（ReturnHome 模板消失），≤8s
    for (int i = 0; i < 20 && !need_exit(); ++i) {
        sleep(400);
        Matcher m(ctrler()->get_image());
        m.set_task_info("StrongholdProtocol@ReturnHome");
        if (!m.analyze()) {
            break;
        }
    }
    board = BoardState {};
    m_current_strategy.clear();
    m_upgrade_x.reset();
    return true;
}
} // namespace asst
