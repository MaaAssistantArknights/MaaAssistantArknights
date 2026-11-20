#include "StrongholdProtocolTaskPlugin.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/StrongholdProtocol/StrongholdProtocolConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Controller/ControllerAPI.h"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

#include <MaaUtils/NoWarningCV.hpp>
#include <MaaUtils/ScopeLeave.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <tuple>

namespace asst
{
namespace stronghold
{
// 界面坐标、roi、偏移量由 pipeline 任务（StrongholdProtocol@Roi@*）的 roi 字段定义，部署格位在 stronghold_protocol
// 资源目录的 tiles.json 中定义；代码里只保留与界面布局无关的色域窗与算法参数（容差、阈值、聚类间距）。

// 色域窗：金 (8,120,150)-(35,255,255)；青（彩标）(70,40,150)-(140,255,255)；
// 绿 = is_green (35,100,100)-(85,255,255)
constexpr int gold_lower_h = 8, gold_upper_h = 35, gold_lower_s = 120;
constexpr int cyan_lower_h = 70, cyan_upper_h = 140, cyan_lower_s = 40;
constexpr int color_lower_v = 150; // 金/青共用 V 下限
constexpr int green_lower_h = 35, green_upper_h = 85, green_lower_s = 100, green_lower_v = 100;
}

using namespace stronghold;

// 所有扫描/判定 ROI 统一从 pipeline 任务的 roi 字段读取；缺任务时记错误并返回全图，避免空 rect 崩溃
static Rect task_roi(const std::string& task_name)
{
    if (auto task = Task.get(task_name); task && task->roi.width > 0 && task->roi.height > 0) {
        return task->roi;
    }
    LogError << __FUNCTION__ << "roi task missing or empty:" << task_name;
    return Rect { 0, 0, 1280, 720 };
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
    // 卡死判定：选定策略、推进到新回合、结算算作进展；
    // 战斗回合与过场落在 Unknown 属正常等待，长时间无进展即停止
    constexpr auto stall_timeout = std::chrono::minutes(10);
    // 离开判定：连续这么久既没有已知场景、也扫不到卫戍协议界面关键词即停止
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
        auto image = ctrler()->get_image();
        UnknownScreen screen;
        Scene scene = detect_scene(image, screen);
        if (scene == Scene::Unknown && screen.generic_button) {
            // 过场中按钮和坐标都可能变化，点击前用新画面重新判定场景。
            image = ctrler()->get_image();
            scene = detect_scene(image, screen);
        }
        if (scene != Scene::Unknown) {
            last_in_garrison = std::chrono::steady_clock::now();
        }
        if (scene != Scene::Settlement && scene != Scene::Unknown) {
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
        case Scene::BountySelect:
            pick_bounty();
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
            prep_phase(board);
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
        default: {
            // 停在选择朝向界面（朝向拖拽没生效）：按最近部署的格子补设朝向
            if (m_last_tile) {
                OCRer facing(image);
                facing.set_task_info("StrongholdProtocol@FacingCancelOcr");
                if (auto r = facing.analyze(); r && !r->empty()) {
                    set_facing(*m_last_tile);
                    break;
                }
            }
            if (screen.in_garrison || screen.in_battle) {
                last_in_garrison = std::chrono::steady_clock::now();
            }
            else if (std::chrono::steady_clock::now() - last_in_garrison > leave_timeout) {
                save_img(utils::path("debug") / utils::path("stronghold"), false);
                LogError << __FUNCTION__ << "stronghold protocol screen not seen for" << leave_timeout.count()
                         << "seconds, task stopped";
                return false;
            }
            if (screen.generic_button) {
                // 仍在卫戍协议内且不在战斗中：点通用按钮推进（继续/确认/领取等）
                LogInfo << __FUNCTION__ << "generic click:" << screen.generic_button->text;
                ctrler()->click(screen.generic_button->rect);
            }
            // 点掉弹窗/过场后短等即重判场景（回合尽快开始）；战斗中无进展可等，慢轮询
            sleep(screen.in_battle ? 800 : 500);
            continue;
        }
        }
        if (progressed) {
            last_progress = std::chrono::steady_clock::now();
        }
        sleep(400);
    }
    return true;
}

static bool has_scene_text(const std::vector<TextRect>& text, const std::string& task_name, size_t required = 1)
{
    const auto task = Task.get<OcrTaskInfo>(task_name);
    if (!task || text.size() < required) {
        return false;
    }
    const auto& config = OcrConfig::get_instance();
    auto keywords = task->text;
    for (auto& keyword : keywords) {
        keyword = config.process_equivalence_class(keyword);
    }
    size_t matches = 0;
    for (const auto& row : text) {
        if (!task->roi.empty() && !task->roi.include(row.rect)) {
            continue;
        }
        const auto line = config.process_equivalence_class(row.text);
        const bool matched = std::ranges::any_of(keywords, [&](const std::string& keyword) {
            return task->full_match ? line == keyword : line.find(keyword) != std::string::npos;
        });
        if (matched && ++matches == required) {
            return true;
        }
    }
    return false;
}

StrongholdProtocolTaskPlugin::Scene
    StrongholdProtocolTaskPlugin::detect_scene(const cv::Mat& image, UnknownScreen& screen, bool wait_for_transition)
{
    const auto match = [&image](const std::string& task_name) {
        Matcher matcher(image);
        matcher.set_task_info(task_name);
        return matcher.analyze().has_value();
    };
    const auto read = [&image](const std::string& task_name) {
        OCRer ocr(image);
        ocr.set_task_info(task_name);
        return ocr.analyze().value_or(std::vector<TextRect> {});
    };
    if (match("StrongholdProtocol@ReturnHome")) {
        return Scene::Settlement;
    }
    const bool squad_button = match("StrongholdProtocol@CustomOpersConfirm");
    const bool info_button = match("StrongholdProtocol@Nav@ReadyScreen");
    const bool ready = squad_button || info_button;
    const bool strategy =
        !ready && (match("StrongholdProtocol@SPRConfirmSelect") || match("StrongholdProtocol@SPAConfirmSelect") ||
                   match("StrongholdProtocol@Goliath"));
    const auto step = [&](const std::vector<TextRect>& text) {
        if (squad_button && has_scene_text(text, "StrongholdProtocol@CustomSquadTitle")) {
            return Scene::CustomSquad;
        }
        if (info_button && has_scene_text(text, "StrongholdProtocol@ConfirmInfo")) {
            return Scene::ConfirmInfo;
        }
        if (strategy && has_scene_text(text, "StrongholdProtocol@StrategyTitle")) {
            return Scene::StrategySelect;
        }
        return Scene::Unknown;
    };
    Scene scene = Scene::Unknown;
    if (ready || strategy) {
        scene = step(read("StrongholdProtocol@SceneHeader"));
    }
    else if (!read("StrongholdProtocol@ReadyButton").empty()) {
        // 确认准备弹窗可能仍露出右上角的商店按钮。
        scene = read("StrongholdProtocol@ConfirmReadyOcr").empty() ? Scene::RoundPrepare : Scene::ConfirmPopup;
    }
    // 悬赏弹层可能仍露出背景页的标题和按钮。面板内只读到一条线索时，用全屏原文核对两行悬赏文字。
    if ((scene != Scene::Unknown || wait_for_transition) && read("StrongholdProtocol@BountyPanel").empty()) {
        return scene;
    }

    const auto text = read("StrongholdProtocol@ScreenTextOcr");
    if (has_scene_text(text, "StrongholdProtocol@BountyTitle", 2)) {
        return Scene::BountySelect;
    }
    if (scene != Scene::Unknown) {
        return scene;
    }
    if (ready) {
        scene = step(text);
        if (scene != Scene::Unknown) {
            return scene;
        }
    }
    else if (strategy) {
        // 旧策略页可能只有歌利亚或确认按钮可见。
        return Scene::StrategySelect;
    }
    if (has_scene_text(text, "StrongholdProtocol@ConfirmReadyOcr")) {
        return Scene::ConfirmPopup;
    }
    if (has_scene_text(text, "StrongholdProtocol@ReadyButton")) {
        return Scene::RoundPrepare;
    }
    if (match("StrongholdProtocol@Nav@EnterQuickly")) {
        return Scene::Navigate;
    }
    screen = scan_unknown_screen(text);
    return Scene::Unknown;
}

StrongholdProtocolTaskPlugin::UnknownScreen
    StrongholdProtocolTaskPlugin::scan_unknown_screen(const std::vector<TextRect>& text)
{
    UnknownScreen screen;
    if (text.empty()) {
        return screen;
    }
    const auto upper = [](std::string word) {
        std::ranges::transform(word, word.begin(), [](char ch) {
            return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        });
        return word;
    };
    std::string all_text;
    for (const auto& tr : text) {
        all_text += upper(tr.text);
    }
    const auto keywords = [](const std::string& task_name) {
        const auto task = Task.get<OcrTaskInfo>(task_name);
        return task ? task->text : std::vector<std::string> {};
    };
    // 词表条目可用 ｢+｣ 连接多个词，须同时出现；英文按大写比较
    const auto contains = [&](const std::string& keyword) {
        size_t begin = 0;
        while (begin <= keyword.size()) {
            const size_t end = std::min(keyword.find('+', begin), keyword.size());
            if (all_text.find(upper(keyword.substr(begin, end - begin))) == std::string::npos) {
                return false;
            }
            begin = end + 1;
        }
        return true;
    };
    screen.in_garrison = std::ranges::any_of(keywords("StrongholdProtocol@Keywords@Garrison"), contains);
    // 重连入口也显示当前回合，旁边的 ｢放弃模拟｣ 能区分入口和战斗画面。
    screen.in_battle =
        !contains("断线重连+放弃模拟") && std::ranges::any_of(keywords("StrongholdProtocol@Keywords@Battle"), contains);
    if (!screen.in_garrison || screen.in_battle) {
        return screen;
    }
    // 只在仍处于卫戍协议界面且不在战斗中时做通用点击，不在界面外乱点
    const auto generic = keywords("StrongholdProtocol@Keywords@Generic");
    const auto exclude = keywords("StrongholdProtocol@Keywords@GenericExclude");
    for (const auto& tr : text) {
        const auto hit = [&tr](const std::string& k) {
            return tr.text.find(k) != std::string::npos;
        };
        if (std::ranges::any_of(generic, hit) && std::ranges::none_of(exclude, hit)) {
            screen.generic_button = tr;
            break;
        }
    }
    return screen;
}

bool StrongholdProtocolTaskPlugin::pick_bounty()
{
    LogTraceFunction;
    // 悬赏三选一：选中中间项中庸，然后点页面下方 ｢确认选择｣ 按钮
    // 三张悬赏卡：BountyTitle roi 内会含标题 + 三张卡名，剔除含 ｢决策｣ 的标题行后按 x 排序取中间
    OCRer cards(ctrler()->get_image());
    cards.set_task_info("StrongholdProtocol@BountyTitle");
    if (auto results = cards.analyze(); results && !results->empty()) {
        std::vector<TextRect> card_rows;
        for (const auto& tr : *results) {
            if (tr.text.find("决策") != std::string::npos) {
                continue;
            }
            if (tr.text.find("悬赏") != std::string::npos) {
                card_rows.emplace_back(tr);
            }
        }
        if (card_rows.size() >= 3) {
            std::ranges::sort(card_rows, [](const TextRect& a, const TextRect& b) { return a.rect.x < b.rect.x; });
            ctrler()->click(card_rows[1].rect);
        }
        else {
            LogWarn << "bounty cards ocr rows" << card_rows.size() << "< 3";
        }
    }
    else {
        LogWarn << "bounty card ocr missed";
    }
    sleep(400);
    // ｢确认选择｣ 按钮 OCR 命中即点
    OCRer confirm(ctrler()->get_image());
    confirm.set_task_info("StrongholdProtocol@BountyConfirmBtn");
    if (auto results = confirm.analyze(); results && !results->empty()) {
        ctrler()->click(results->front().rect);
    }
    else {
        LogWarn << "bounty confirm ocr missed";
    }
    sleep(800);
    LogInfo << "bounty picked (middle)";
    return true;
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

// ---------------- 颜色判定 ----------------

double StrongholdProtocolTaskPlugin::color_ratio(
    const cv::Mat& image,
    const Rect& roi,
    int lower_h,
    int upper_h,
    int lower_s,
    int lower_v)
{
    // 偏移量算出的 roi 可能越出画面，裁到图内防 cv 越界
    const cv::Rect rect = make_rect<cv::Rect>(roi) & cv::Rect(0, 0, image.cols, image.rows);
    if (rect.empty()) {
        return 0.0;
    }
    cv::Mat hsv;
    cv::cvtColor(image(rect), hsv, cv::COLOR_BGR2HSV);
    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(lower_h, lower_s, lower_v), cv::Scalar(upper_h, 255, 255), mask);
    return static_cast<double>(cv::countNonZero(mask)) / mask.total();
}

bool StrongholdProtocolTaskPlugin::has_gold_badge(const cv::Mat& image, int cx, int cy)
{
    // 角标相对格心的偏移矩形由 Roi@GoldBadgeOffset 任务给出
    const Rect off = task_roi("StrongholdProtocol@Roi@GoldBadgeOffset");
    const Rect roi { cx + off.x, cy + off.y, off.width, off.height };
    return color_ratio(image, roi, gold_lower_h, gold_upper_h, gold_lower_s, color_lower_v) > 0.10;
}

bool StrongholdProtocolTaskPlugin::has_color_badge(const cv::Mat& image, int cx, int cy)
{
    const Rect off = task_roi("StrongholdProtocol@Roi@ColorBadgeOffset");
    const Rect roi { cx + off.x, cy + off.y, off.width, off.height };
    return color_ratio(image, roi, cyan_lower_h, cyan_upper_h, cyan_lower_s, color_lower_v) > 0.15 &&
           color_ratio(image, roi, gold_lower_h, gold_upper_h, gold_lower_s, color_lower_v) < 0.05;
}

bool StrongholdProtocolTaskPlugin::is_green_tile(const cv::Mat& image, const Point& pt, int radius)
{
    return color_ratio(
               image,
               Rect { pt.x - radius, pt.y - radius, radius * 2, radius * 2 },
               green_lower_h,
               green_upper_h,
               green_lower_s,
               green_lower_v) > 0.8;
}

std::optional<int> StrongholdProtocolTaskPlugin::measure_grid_shift(const cv::Mat& before, const cv::Mat& after)
{
    // 用滚动前的一条横带（Roi@StrategyGridStrip）在滚动后的网格可见区（Roi@StrategyGrid）里找位置，
    // 差值即滚动位移（向上为正）
    const cv::Rect grid_cv = make_rect<cv::Rect>(task_roi("StrongholdProtocol@Roi@StrategyGrid"));
    const cv::Rect strip_cv = make_rect<cv::Rect>(task_roi("StrongholdProtocol@Roi@StrategyGridStrip"));
    const cv::Mat strip = before(strip_cv);
    const cv::Mat area = after(grid_cv);
    if (area.rows < strip.rows || area.cols < strip.cols) {
        return std::nullopt;
    }
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

bool StrongholdProtocolTaskPlugin::has_chinese(const std::string& text)
{
    // UTF-8 汉字大多落在 E4..E9 开头的三字节序列，按首字节粗判足够过滤英文/数字
    for (size_t i = 0; i + 1 < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c >= 0xE4 && c <= 0xE9) {
            return true;
        }
    }
    return false;
}

// ---------------- 局外：选策略 / 自选编队 ----------------

StrongholdProtocolTaskPlugin::StrategyResult StrongholdProtocolTaskPlugin::pick_strategy(Scene& next_scene)
{
    LogTraceFunction;

    constexpr int track_top = 111, track_bottom = 689;
    auto scrollbar = [](const cv::Mat& image) -> std::optional<cv::Rect> {
        // 打开详情后网格和滚动条一起左移。窄条内只有滑块会形成两侧留空的灰白色长条。
        for (const int x : { 956, 1220 }) {
            const cv::Rect strip(x - 7, track_top - 6, 15, track_bottom - track_top + 13);
            if ((strip & cv::Rect(0, 0, image.cols, image.rows)) != strip) {
                continue;
            }
            cv::Mat hsv, mask;
            cv::cvtColor(image(strip), hsv, cv::COLOR_BGR2HSV);
            cv::inRange(hsv, cv::Scalar(0, 0, 100), cv::Scalar(180, 32, 255), mask);
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (const auto& contour : contours) {
                const auto rect = cv::boundingRect(contour);
                if (rect.height >= 48 && rect.width >= 5 && rect.width <= 10 &&
                    std::abs(rect.x + rect.width / 2 - 7) <= 1 && cv::contourArea(contour) > rect.area() * 0.7) {
                    return rect + strip.tl();
                }
            }
        }
        return std::nullopt;
    };

    if (!ProcessTask(*this, { "StrongholdProtocol@StrategyGridProbe" }).run()) {
        return StrategyResult::IncompleteScan;
    }

    auto detect_card_frames = [](const cv::Mat& img, const cv::Rect& thumb, bool& verified) {
        // 已知网格位置时按四列卡框核对，暗色立绘和无角标策略也有左边框。
        constexpr int view_top = 93, view_bottom = 708;
        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        std::vector<Point> cells;
        for (int offset : { 566, 426, 286, 146 }) {
            const int left = thumb.x + thumb.width / 2 - offset;
            const auto band = gray.rowRange(view_top, view_bottom);
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
                edge_contrast(edge, column_mean(left + 6)) &
                edge_contrast(column_mean(left + 124), column_mean(left + 117)));
            for (const auto& rect : inner_edges) {
                if (rect.y > 0 && rect.br().y < band.rows && rect.height >= 108 && rect.height <= 136 &&
                    std::ranges::none_of(
                        borders,
                        [&](const cv::Rect& found) {
                            return found.height >= 108 && found.height <= 136 &&
                                   std::abs(found.br().y - rect.br().y) < 35;
                        })) {
                    borders.emplace_back(rect);
                }
            }
            for (const auto& rect : borders) {
                if (rect.y == 0 || rect.br().y == band.rows) {
                    // 相邻页有重叠，裁断的卡留给显示全卡的一页。首尾再有裁断则无法核全。
                    if (rect.height >= 30 && ((rect.y == 0 && thumb.y <= track_top + 2) ||
                                              (rect.br().y == band.rows && thumb.br().y >= track_bottom - 2))) {
                        verified = false;
                    }
                    continue;
                }
                if (rect.height >= 108 && rect.height <= 136) {
                    cells.emplace_back(left + 64, view_top + rect.br().y - 65);
                }
                else if (rect.height >= 30) {
                    verified = false;
                }
            }
        }
        // 分组末行可以留空。若已识别卡的左邻缺失，无法确认整行都已核对。
        const int first_col = thumb.x + thumb.width / 2 - 566 + 64;
        for (const auto& cell : cells) {
            if (cell.x > first_col && std::ranges::none_of(cells, [&](const Point& other) {
                    return other.x == cell.x - 140 && std::abs(other.y - cell.y) < 35;
                })) {
                verified = false;
                break;
            }
        }
        return cells;
    };

    // 按立绘和角标定位可选卡，即使卡框或滑块识别不全也能继续选策略。
    auto detect_grid_cells = [](const cv::Mat& img) {
        // 两种布局固定列心的并集（720p，全屏坐标）
        static constexpr int kCols[] = { 453, 593, 722, 733, 862, 873, 1002, 1142 };
        // 标记块色相段（OpenCV H 0-180）与块中心相对卡心的偏移：红三角在卡左上、其余在卡顶右
        struct Seg
        {
            int h_lo, h_hi, ox, oy;
        };
        static constexpr Seg kSegs[] = {
            { 166, 180, -34, 68 }, // NEW
            { 13, 22, -47, 51 },   // 金章
            { 73, 108, -47, 51 },  // 彩章
            { 0, 6, 45, 48 },      // 红三角（禁用标记）
        };
        const Rect grid = task_roi("StrongholdProtocol@Roi@StrategyGrid");
        const cv::Rect grid_cv = make_rect<cv::Rect>(grid);
        cv::Mat hsv;
        cv::cvtColor(img(grid_cv), hsv, cv::COLOR_BGR2HSV);

        struct Ev
        {
            double x, y;
        };
        std::vector<Ev> evidence;

        // 立绘亮像素块：闭运算连接素描纹理线条，垂直长核腐蚀断开跨卡的水平连接
        cv::Mat lit;
        cv::inRange(hsv, cv::Scalar(0, 0, 96), cv::Scalar(180, 109, 255), lit);
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

        for (const auto& seg : kSegs) {
            cv::Mat m;
            cv::inRange(hsv, cv::Scalar(seg.h_lo, 101, 121), cv::Scalar(seg.h_hi, 255, 255), m);
            cv::erode(m, m, cv::getStructuringElement(cv::MORPH_RECT, { 3, 3 }));
            std::vector<std::vector<cv::Point>> cs;
            cv::findContours(m, cs, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (const auto& c : cs) {
                const auto rc = cv::boundingRect(c);
                if (rc.width * rc.height < 150 || rc.width > 90 || rc.height > 60) {
                    continue;
                }
                evidence.emplace_back(
                    grid_cv.x + rc.x + rc.width / 2.0 + seg.ox,
                    grid_cv.y + rc.y + rc.height / 2.0 + seg.oy);
            }
        }

        // 列心吸附过滤杂散（顶栏图标/左栏暗红横幅/详情面板亮块的证据落不到列心附近），再按卡去重
        std::ranges::sort(evidence, [](const Ev& a, const Ev& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
        std::vector<Ev> dedup;
        for (const auto& e : evidence) {
            double best = 1e9;
            for (int c : kCols) {
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
                int best_col = kCols[0];
                for (int c : kCols) {
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

    const Rect title_band = task_roi("StrongholdProtocol@Roi@StrategyPanelTitle");
    const Rect name_band = task_roi("StrongholdProtocol@Roi@StrategyPanelName");
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
    for (int page = 0; page < max_pages && !need_exit(); ++page) {
        if (image.cols < 1280 || image.rows < 720) {
            LogWarn << "strategy screenshot unavailable";
            return StrategyResult::IncompleteScan;
        }
        auto cells = detect_grid_cells(image);
        if (thumb) {
            auto frames = detect_card_frames(image, *thumb, verified);
            verified &= !frames.empty();
            // 保留未能核对卡框的立绘候选。裁切到视窗边缘的卡由相邻页覆盖，侧栏不属于网格。
            const int bar_x = thumb->x + thumb->width / 2;
            std::erase_if(cells, [&](const Point& pt) {
                return pt.x < bar_x - 550 || pt.x > bar_x ||
                       (!frames.empty() && ((pt.y < 150 && thumb->y > track_top + 2) ||
                                            (pt.y > 650 && thumb->br().y < track_bottom - 2))) ||
                       std::ranges::any_of(frames, [&](const Point& frame) {
                           return std::abs(pt.x - frame.x) < 25 && std::abs(pt.y - frame.y) < 45;
                       });
            });
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
            // 选中卡的青色边框也会进入彩标搜索区，再核对角标内部。
            const Rect badge_center { pt.x + 32, pt.y - 60, 26, 28 };
            if ((has_color_badge(image, pt.x, pt.y) &&
                 color_ratio(image, badge_center, cyan_lower_h, cyan_upper_h, cyan_lower_s, color_lower_v) > 0.15) ||
                (m_target <= 3 && has_gold_badge(image, pt.x, pt.y))) {
                continue;
            }

            // 点击可能留在上一张卡的详情。面板读数可用于挑选，只有当前卡的角标能独立证明它已达标。
            verified = false;
            if (!ctrler()->click(Point { pt.x + grid_offset, pt.y }) || !sleep(400)) {
                return StrategyResult::IncompleteScan;
            }

            // 右侧面板读取：策略名 + 已通过次数；面板有切换动画，读不到名字重试
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
                OCRer ocr(panel);
                ocr.set_task_info("StrongholdProtocol@StrategyPanel");
                auto results = ocr.analyze();
                if (!results || results->empty()) {
                    sleep(300);
                    continue;
                }
                // 已通过次数在面板标题区（Roi@StrategyPanelTitle 的 y 区间）；｢通过｣ 与次数可能被拆成两行。
                // 无 ｢通过｣ 行即未打过（0 次），有 ｢通过｣ 但读不到数字按 1 次
                passed = 0;
                std::vector<const TextRect*> title_rows;
                for (const auto& tr : *results) {
                    if (tr.rect.y >= title_band.y && tr.rect.y < title_band.y + title_band.height) {
                        title_rows.emplace_back(&tr);
                    }
                }
                auto normalize_digits = [](std::string text) {
                    // ① 等带圈数字会被 OCR 混入，按 UTF-8 字节序列整体替换为 ASCII 数字
                    static constexpr std::pair<std::string_view, char> special[] = {
                        { "\xE2\x91\xA0", '1' }, { "\xE2\x91\xA1", '2' }, { "\xE2\x91\xA2", '3' },
                        { "\xE2\x91\xA3", '4' }, { "\xE2\x91\xA4", '5' }, { "\xE2\x91\xA5", '6' },
                        { "\xE2\x91\xA6", '7' }, { "\xE2\x91\xA7", '8' }, { "\xE2\x91\xA8", '9' },
                    };
                    for (auto [from, to] : special) {
                        for (size_t pos = text.find(from); pos != std::string::npos; pos = text.find(from)) {
                            text.replace(pos, from.size(), 1, to);
                        }
                    }
                    std::string digits;
                    for (char c : text) {
                        if (c >= '0' && c <= '9') {
                            digits += c;
                        }
                    }
                    return digits;
                };
                for (size_t i = 0; i < title_rows.size(); ++i) {
                    const auto& text = title_rows[i]->text;
                    if (text.find("通过") == std::string::npos) {
                        continue;
                    }
                    std::string digits = normalize_digits(text.substr(text.find("通过") + 6));
                    if (digits.empty() && i + 1 < title_rows.size()) {
                        digits = normalize_digits(title_rows[i + 1]->text);
                    }
                    // 次数至多两位，截断防 OCR 粘连导致 stoi 溢出
                    passed = digits.empty() ? 1 : std::stoi(digits.substr(0, 2));
                    break;
                }
                // 策略名在面板中下部（Roi@StrategyPanelName 的 y 区间），排除说明行（生命值/通过/方括号）
                for (const auto& tr : *results) {
                    if (tr.rect.y < name_band.y || tr.rect.y > name_band.y + name_band.height) {
                        continue;
                    }
                    if (!has_chinese(tr.text)) {
                        continue;
                    }
                    if (tr.text.find("生命") != std::string::npos || tr.text.find("通过") != std::string::npos ||
                        tr.text.find("【") != std::string::npos) {
                        continue;
                    }
                    strategy_name = tr.text;
                    break;
                }
                if (!strategy_name.empty()) {
                    break;
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
                UnknownScreen screen;
                // 过场先查局部特征，画面仍未确定时再走完整识别。
                scene = detect_scene(ctrler()->get_image(), screen, attempt < 9);
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
        if (thumb && next_thumb) {
            const int advance = next_thumb->y - thumb->y;
            if (advance <= 0) {
                LogWarn << "strategy list did not advance" << advance;
                return StrategyResult::IncompleteScan;
            }
            if (verified) {
                // 滑块先到位时卡片可能还停在原页。用上一页底部一张卡高的内容确认两页确实重叠。
                constexpr int view_top = 93, view_height = 615, overlap_height = 136, grid_width = 546;
                const int previous_left = thumb->x + thumb->width / 2 - 566;
                const int next_left = next_thumb->x + next_thumb->width / 2 - 566;
                const int last_y = view_height - overlap_height;
                const int min_shift = std::max(1, advance * (track_bottom - track_top) / thumb->height - 4);
                const int max_shift = std::min(last_y, advance * view_height / thumb->height + 4);
                verified = std::abs(next_thumb->height - thumb->height) <= 2 && min_shift <= max_shift;
                if (verified) {
                    const cv::Mat strip = image(cv::Rect(previous_left, view_top + last_y, grid_width, overlap_height));
                    const cv::Mat grid = after(cv::Rect(next_left, view_top, grid_width, view_height));
                    cv::Mat scores;
                    cv::matchTemplate(grid, strip, scores, cv::TM_CCOEFF_NORMED);
                    double moved = 0;
                    cv::minMaxLoc(scores.rowRange(last_y - max_shift, last_y - min_shift + 1), nullptr, &moved);
                    verified = moved >= 0.8 && moved > scores.at<float>(last_y, 0) + 0.001;
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
        std::vector<TextRect> names;
        for (const auto& tr : *text) {
            if (!BattleData.get_all_oper_names().contains(tr.text) || StrongholdProtocol.is_substitute(tr.text) ||
                !StrongholdProtocol.is_six_star(tr.text)) {
                continue;
            }
            const int x = tr.rect.x + tr.rect.width / 2;
            const bool banned = std::ranges::any_of(*text, [&](const TextRect& mark) {
                return mark.text.find("禁用") != std::string::npos && mark.rect.y > tr.rect.y &&
                       mark.rect.y - tr.rect.y < 80 && std::abs(x - mark.rect.x - mark.rect.width / 2) < 60;
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
    const auto& number_replace = Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map;
    const auto& task_replace = Task.get<OcrTaskInfo>(task_name)->replace_map;
    std::decay_t<decltype(task_replace)> merged_replace {};
    merged_replace.reserve(number_replace.size() + task_replace.size());
    std::ranges::copy(number_replace, std::back_inserter(merged_replace));
    std::ranges::copy(task_replace, std::back_inserter(merged_replace));

    RegionOCRer ocr(image);
    ocr.set_task_info(task_name);
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
            if (tr.text.find("满级") != std::string::npos) {
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

static std::vector<int>
    detect_shop_card_x(const std::vector<TextRect>& ocr_results, const Rect& cost_band, const Rect& tier_band)
{
    // 费用徽章在卡槽正中，等阶框与它的间距固定。左对齐的名字会随字数改变中心位置。
    std::vector<int> centers;
    for (const auto& tr : ocr_results) {
        const int cx = tr.rect.x + tr.rect.width / 2;
        const int cy = tr.rect.y + tr.rect.height / 2;
        if (cy >= cost_band.y && cy <= cost_band.y + cost_band.height) {
            centers.emplace_back(cx);
        }
        else if (cy >= tier_band.y && cy <= tier_band.y + tier_band.height) {
            centers.emplace_back(cx + tier_band.x);
        }
    }
    std::ranges::sort(centers);
    std::vector<int> slots;
    for (int x : centers) {
        if (slots.empty() || x - slots.back() > 12) {
            slots.emplace_back(x);
        }
        else {
            slots.back() = (slots.back() + x) / 2;
        }
    }
    return slots;
}

std::vector<StrongholdProtocolTaskPlugin::ShopCard>
    StrongholdProtocolTaskPlugin::read_shop_cards(const cv::Mat& image, const std::vector<ShopCard>& previous) const
{
    OCRer ocr(image);
    ocr.set_task_info("StrongholdProtocol@ShopCardsOcr");
    auto results_opt = ocr.analyze();

    const auto& oper_names = BattleData.get_all_oper_names();
    const auto parse_cost = [](std::string_view text) -> std::optional<int> {
        if (text == "o" || text == "O") {
            return 0;
        }
        unsigned value = 0;
        if (text.size() <= 2 && utils::chars_to_number<unsigned, true>(text, value)) {
            return static_cast<int>(value);
        }
        return std::nullopt;
    };
    const auto tier_roi = task_roi("StrongholdProtocol@Roi@ShopTierY");
    const auto cost_roi = task_roi("StrongholdProtocol@Roi@ShopCostY");
    auto card_xs = results_opt ? detect_shop_card_x(*results_opt, cost_roi, tier_roi) : std::vector<int> {};
    std::vector<ShopCard> cards;
    static const std::unordered_map<std::string, int> roman = {
        { "I", 1 }, { "II", 2 }, { "III", 3 }, { "IV", 4 }, { "V", 5 }, { "VI", 6 },
    };
    auto center = [](const Rect& r) {
        return r.x + r.width / 2;
    };
    auto center_y = [](const Rect& r) {
        return r.y + r.height / 2;
    };
    const auto empty_roi = task_roi("StrongholdProtocol@Roi@ShopEmptyY");
    const auto pact_roi = task_roi("StrongholdProtocol@Roi@ShopPactY");
    const auto name_roi2 = task_roi("StrongholdProtocol@Roi@ShopNameY");
    // 单卡局部区域：x 为相对卡牌中心的偏移，y/w/h 为绝对值
    const auto tier_frame = task_roi("StrongholdProtocol@Roi@ShopTierFrame");
    const auto cost_reread = task_roi("StrongholdProtocol@Roi@ShopCostReread");
    const auto card_rect = [](const Rect& r, int card_x) {
        return Rect { card_x + r.x, r.y, r.width, r.height };
    };
    const auto read_badge = [&](int x) {
        RegionOCRer cost(image);
        cost.set_roi(card_rect(cost_reread, x));
        const auto result = cost.analyze();
        return result ? parse_cost(result->text).value_or(-1) : -1;
    };
    const int tier_lo = tier_roi.y, tier_hi = tier_roi.y + tier_roi.height;
    const int cost_lo = cost_roi.y, cost_hi = cost_roi.y + cost_roi.height;
    const int empty_lo = empty_roi.y, empty_hi = empty_roi.y + empty_roi.height;
    const int pact_lo = pact_roi.y, pact_hi = pact_roi.y + pact_roi.height;
    const int name_lo2 = name_roi2.y, name_hi2 = name_roi2.y + name_roi2.height;
    std::map<int, std::string> recovered;
    if (results_opt && !previous.empty()) {
        // 整架 OCR 可能连续漏掉费用和等阶。全名仍在原处时，用它的位移修正上次确认的卡槽中心。
        for (const auto& old : previous) {
            if (!old.is_oper || old.cost <= 0 || old.name_left < 0 || old.name.empty()) {
                continue;
            }
            for (const auto& tr : *results_opt) {
                const int dx = tr.rect.x - old.name_left;
                const int cy = center_y(tr.rect);
                if (tr.text != old.name || std::abs(dx) > 8 || cy < name_lo2 || cy > name_hi2) {
                    continue;
                }
                const int x = old.x + dx;
                if (std::ranges::none_of(card_xs, [&](int current) { return std::abs(current - x) <= 60; })) {
                    recovered.emplace(x, old.name);
                    card_xs.emplace_back(x);
                }
                break;
            }
        }
        std::ranges::sort(card_xs);
    }
    if (m_upgrade_x) {
        const int span = task_roi("StrongholdProtocol@Roi@ShopUpgradeButtonSpan").width;
        std::erase_if(card_xs, [&](int x) { return x <= *m_upgrade_x + span; });
    }
    for (int x : card_xs) {
        ShopCard card;
        card.x = x;
        if (results_opt) {
            for (const auto& tr : *results_opt) {
                const int cx = center(tr.rect);
                const int cy = center_y(tr.rect);
                // 卡宽约 110，水平容差 60
                if (std::abs(cx - x) > 60) {
                    continue;
                }
                LogTrace << "shop tr card" << x << "text" << tr.text << "cx" << cx << "cy" << cy;
                std::string upper;
                std::ranges::transform(tr.text, std::back_inserter(upper), [](char c) {
                    return c == '1' || c == 'L' || c == 'l'
                               ? 'I'
                               : static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                });
                // 等阶罗马数字：左上黑牌区，x 偏卡左（cx < x）
                if (cy >= tier_lo && cy <= tier_hi && cx < x && roman.contains(upper)) {
                    card.tier = roman.at(upper);
                    continue;
                }
                if (cy >= cost_lo && cy <= cost_hi && std::abs(cx - x) <= cost_reread.width / 2) {
                    if (const auto cost = parse_cost(tr.text)) {
                        card.cost = *cost;
                        continue;
                    }
                }
                if (cy >= empty_lo && cy <= empty_hi && tr.text.find("NO INFO") != std::string::npos) {
                    card.empty = true;
                    continue;
                }
                if (cy >= pact_lo && cy <= pact_hi && has_chinese(tr.text)) {
                    card.is_oper = true;
                    continue;
                }
                if (cy >= name_lo2 && cy <= name_hi2 && card.name.empty() && oper_names.contains(tr.text)) {
                    card.name = tr.text;
                    card.name_left = tr.rect.x;
                    LogTrace << "shop name matched" << tr.text << "card" << x;
                }
            }
        }
        if (const auto it = recovered.find(x); it != recovered.end()) {
            if (card.empty || !card.is_oper || card.name != it->second) {
                continue;
            }
            card.cost = read_badge(x);
            if (card.cost <= 0) {
                continue;
            }
            LogTrace << "recovered shop slot" << x << card.name << "cost" << card.cost;
        }
        else if (card.cost < 0 && !card.empty) {
            card.cost = read_badge(x);
        }
        // 等阶未识别时按卡牌左上金框色判 V/VI
        if (card.tier == 0 &&
            color_ratio(image, card_rect(tier_frame, x), gold_lower_h, gold_upper_h, gold_lower_s, color_lower_v) >
                0.1) {
            card.tier = 5;
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
                const int cost = read_badge(card.x);
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

bool StrongholdProtocolTaskPlugin::can_buy_medic(const BoardState& board, int tier)
{
    const auto field = field_medic_tier(board);
    return improves_field_medic(field, tier) && !std::ranges::any_of(board.bench_known, [&](const auto& copy) {
               return is_medic(copy.first) && improves_field_medic(field, copy.second);
           });
}

bool StrongholdProtocolTaskPlugin::is_reward_shop(const std::vector<ShopCard>& cards)
{
    return std::ranges::count_if(cards, [](const ShopCard& card) {
               return card.is_oper && !card.empty && card.cost == 0;
           }) == 3;
}

std::optional<StrongholdProtocolTaskPlugin::ShopCard>
    StrongholdProtocolTaskPlugin::promotion_reward(const std::vector<ShopCard>& cards, const BoardState& board)
{
    return is_reward_shop(cards) ? shop_pick(cards, board, 0) : std::nullopt;
}

std::optional<StrongholdProtocolTaskPlugin::ShopCard>
    StrongholdProtocolTaskPlugin::shop_pick(const std::vector<ShopCard>& cards, const BoardState& board, int gold)
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
        const auto role = StrongholdProtocol.get_role(card.name);
        const bool suitable =
            role != battle::Role::Unknown && (role != battle::Role::Medic || can_buy_medic(board, card.tier));
        if (!free && !card.name.empty() && !suitable) {
            continue;
        }
        const auto copies = board.purchases.find(card.name);
        // 免费三选一必须领走一张。有合适的赠卡时才跳过多余的医疗。
        const auto score = std::tuple { free,
                                        !free || suitable,
                                        rank(card),
                                        missing_medic && suitable && role == battle::Role::Medic,
                                        copies == board.purchases.end() ? 0 : copies->second.unmerged };
        if (!best || score > best_score) {
            best = card;
            best_score = score;
        }
    }
    return best;
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
    const auto can_buy = [&] {
        const auto role = StrongholdProtocol.get_role(card.name);
        return reward ||
               (role != battle::Role::Unknown && (role != battle::Role::Medic || can_buy_medic(board, card.tier)));
    };
    if (!card.name.empty() && !can_buy()) {
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
            if (card.name.empty()) {
                // 卡面漏字时，详情面板只显示当前选中的干员。
                OCRer name(img);
                name.set_task_info("StrongholdProtocol@DragNameOcr");
                if (const auto rows = name.analyze()) {
                    for (const auto& row : *rows) {
                        if (BattleData.get_all_oper_names().contains(row.text)) {
                            card.name = row.text;
                            break;
                        }
                    }
                }
                if (card.name.empty() && !reward) {
                    continue;
                }
            }
            break;
        }
    }
    if (saw_no_funds) {
        ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
        return PurchaseResult::Failed;
    }
    if (!saw_trait || !can_buy()) {
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
        const auto role = StrongholdProtocol.get_role(c.name);
        return c.is_oper && !c.empty && c.cost > 0 && (c.name.empty() || role != battle::Role::Unknown) &&
               (role != battle::Role::Medic || can_buy_medic(board, c.tier));
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
        else if (phase == ShopPhase::Early) {
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
        board.medic_bench.clear(); // 合成或买入可能在同一席位换人。
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
        std::erase_if(board.opening_gifts, material);
        if (opening) {
            std::erase_if(*opening, material);
        }
        if (!card.name.empty()) {
            auto& copies = board.purchases.at(card.name);
            copies.unmerged = std::max(0, copies.unmerged - 3);
            for (const auto& [key, occupant] : board.occ) {
                if (occupant.first == card.name) {
                    board.promotion_tiles.emplace(key);
                }
            }
            for (auto& [name, tier] : board.bench_known) {
                if (name == card.name) {
                    tier = 0;
                }
            }
            board.bench_known.emplace_back(card.name, 0);
        }
        const auto reward = shop_pick(cards_after, board, 0);
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

void StrongholdProtocolTaskPlugin::deploy_opening(BoardState& board, std::vector<OpeningOper>& opening, bool gifts)
{
    std::erase_if(opening, [&](const OpeningOper& oper) {
        const auto uncertain = board.unconfirmed_tiles.find(oper.tile);
        const auto copies = std::ranges::count(board.bench_known, oper.name, &std::pair<std::string, int>::first);
        // 唯一持有的赠卡若仍能从席上拿起，可再试它原先的空格。其他干员不能借此覆盖待核格。
        const bool retry_gift = gifts && uncertain != board.unconfirmed_tiles.end() && uncertain->second == oper.name &&
                                !board.occ.contains(oper.tile) && !board.purchases.contains(oper.name) && copies == 1;
        const bool release_gift =
            gifts &&
            ((uncertain != board.unconfirmed_tiles.end() && !retry_gift) ||
             (copies > 1 && board.purchases.contains(oper.name)) ||
             (board.occ.contains(oper.tile) && !is_medic(oper.name)) ||
             (is_medic(oper.name) && !retry_gift && !improves_field_medic(field_medic_tier(board), oper.tier)));
        if (oper.tile.empty() || release_gift) {
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
            if (retry_gift) {
                board.unconfirmed_tiles.erase(oper.tile);
            }
            if (deploy_from_bench(board, p, oper.name, oper.tier, {}, oper.tile)) {
                board.medic_bench.clear();
                return true;
            }
            if (retry_gift) {
                board.unconfirmed_tiles.try_emplace(oper.tile, oper.name);
            }
            return false;
        };
        if (!retry_gift && oper.bench && try_slot(*oper.bench)) {
            return true;
        }
        // 再买一人或部署上一人后，原格可能已换人。只有面板姓名一致才能使用预定的站位。
        const auto current = detect_bench_opers(ctrler()->get_image());
        return std::ranges::any_of(current, [&](const Point& p) {
            return (retry_gift || !oper.bench || Point::distance(p, *oper.bench) >= 8) && try_slot(p);
        });
    });
    board.pending_deploy |= !opening.empty();
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
    const bool deferred_opening = m_current_strategy == "老鲤";
    if (deferred_opening && round == 3 && board.round_done < 3) {
        // 老鲤在第 3 回合休整时发放暂存资金，并将两名赠送干员直接放入备战席。
        board.unread_opening_gifts = 2;
    }
    if (board.unread_opening_gifts > 0 || !board.opening_gifts.empty()) {
        if (board.unread_opening_gifts > 0) {
            deploy_opening_gifts(board, image);
        }
        deploy_opening(board, board.opening_gifts, true);
        image = ctrler()->get_image();
        shop = read_shop(image, board);
    }
    else if ((board.round_done >= 0 || round > 1) && deploy_bench_medic(board, image)) {
        image = ctrler()->get_image();
        shop = read_shop(image, board);
    }
    if (!deferred_opening && board.round_done < 0 && round == 1) {
        int remaining = 2;
        int budget = shop.gold.value_or(-1);
        std::vector<int> failed_slots;
        std::vector<ShopCard> previous;
        while (remaining > 0 && !need_exit()) {
            auto cards = read_shop_cards(image, previous);
            const bool reward_shop = is_reward_shop(cards);
            std::erase_if(cards, [&](const ShopCard& c) {
                return (reward_shop && c.cost != 0) ||
                       std::ranges::any_of(failed_slots, [&](int x) { return std::abs(c.x - x) < 30; });
            });
            auto pick = reward_shop ? shop_pick(cards, board, 0) : opening_pick(cards, budget, remaining, board);
            if (!pick) {
                break;
            }
            previous = std::move(cards);
            std::erase_if(previous, [&](const ShopCard& c) { return reward_shop || std::abs(c.x - pick->x) < 30; });
            if (buy_and_deploy(board, *pick, ShopPhase::Early, &opening) == PurchaseResult::Bought) {
                --remaining;
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
        assign_opening_tiles(opening);
        deploy_opening(board, opening);
        image = ctrler()->get_image();
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
                if (!rows || (after_confirm && rows->front().text == "确认升级")) {
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
            auto roi = task->roi;
            roi.x += button.rect.x + button.rect.width / 2;
            OCRer ocr(image);
            ocr.set_task_info(task);
            ocr.set_roi(roi);
            const auto rows = ocr.analyze();
            if (rows && rows->size() == 1 && rows->front().score >= 0.75) {
                const auto it = std::ranges::find(task->text, rows->front().text);
                if (it != task->text.end()) {
                    return static_cast<int>(it - task->text.begin()) + 1;
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
            if (button->text == "确认升级") {
                confirm.set_reusable_image(image);
            }
            else if (button->text != "升级" || !ctrler()->click(button->rect)) {
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
                const auto slots = slots_left(image);
                if (gold == 0) {
                    break;
                }
                if ((slots == 0 && !is_medic(card.name)) || !card.is_oper || card.cost < 0 ||
                    (gold && card.cost > *gold)) {
                    continue;
                }
                buy_and_deploy(board, card, ShopPhase::Early, opening.empty() ? nullptr : &opening);
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
            if (const auto pick = shop_pick(cards, board, gold.value_or(99))) {
                LogInfo << "buy V/VI:" << pick->name << "cost" << pick->cost;
                const auto result =
                    buy_and_deploy(board, *pick, ShopPhase::Maxed, opening.empty() ? nullptr : &opening);
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
        deploy_opening(board, board.opening_gifts, true);
        if (opening.empty()) {
            board.pending_deploy = deploy_bench(board);
        }
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
    // 多结果 OCRer：弹窗 roi 内多行文本，单结果模式可能取到正文行漏判
    OCRer popup(ctrler()->get_image());
    popup.set_task_info("StrongholdProtocol@ConfirmReadyOcr");
    if (popup.analyze()) {
        ProcessTask(*this, { "StrongholdProtocol@PopupNotAgain" }).run();
        ProcessTask(*this, { "StrongholdProtocol@PopupConfirmBtn" }).run();
    }
}

// ---------------- 局内：部署 ----------------

// minitouch 的 d/m/u 须后跟 c（commit）才提交到触摸设备，控制器注入时不自动带，
// 触摸事件统一经此入口补发 COMMIT；Mumu 后端对 COMMIT 无操作（即时生效），同样兼容。
// 等待在主线程同步 sleep（同 MinitouchController::swipe 收尾等队列消费完的语义）：
// 设备侧排队的 w 命令异步执行，注入返回时游戏尚未推进到对应时刻，
// 紧随的截图会拿到超前画面
void StrongholdProtocolTaskPlugin::emit_input(InputEvent::Type type, const Point& p, long ms)
{
    if (type == InputEvent::Type::WAIT_MS) {
        sleep(static_cast<int>(ms));
        return;
    }
    InputEvent e;
    e.type = type;
    e.point = p;
    e.milisec = ms;
    ctrler()->inject_input_event(e);
    switch (type) {
    case InputEvent::Type::TOUCH_DOWN:
    case InputEvent::Type::TOUCH_MOVE:
    case InputEvent::Type::TOUCH_UP:
        e.type = InputEvent::Type::COMMIT;
        ctrler()->inject_input_event(e);
        break;
    default:
        break;
    }
}

// 按住拖拽：游戏要求先在干员头像上 press-and-hold 进入拖拽预览，再滑动到目标格。
// swipe 的快速位移来不及触发 grab，故用 inject_input_event 手动 down→hold→move→hold→up。
void StrongholdProtocolTaskPlugin::drag_to(const Point& from, const Point& to, int hold_ms, int move_ms)
{
    emit_input(InputEvent::Type::TOUCH_DOWN, from);
    emit_input(InputEvent::Type::WAIT_MS, from, hold_ms); // 按住让游戏进入拖拽预览
    drop_held(from, to, move_ms, hold_ms / 2);
}

void StrongholdProtocolTaskPlugin::drop_held(const Point& from, const Point& to, int move_ms, int settle_ms)
{
    // 控制器注入的 TOUCH_MOVE 不保证 milisec 生效（瞬时连发），每步 MOVE 后须插 WAIT_MS
    // 让游戏帧感知位移，否则整个拖拽被压缩成一帧判定不到
    const int steps = 8;
    const int per = std::max(16, move_ms / steps);
    for (int i = 1; i <= steps; ++i) {
        emit_input(
            InputEvent::Type::TOUCH_MOVE,
            Point { from.x + (to.x - from.x) * i / steps, from.y + (to.y - from.y) * i / steps });
        emit_input(InputEvent::Type::WAIT_MS, to, per);
    }
    emit_input(InputEvent::Type::WAIT_MS, to, settle_ms); // 终点按住让游戏确认落格
    emit_input(InputEvent::Type::TOUCH_UP, to);
}

static bool is_tier_one_stroke(const cv::Mat& image, const cv::Rect& inner, int threshold = 140)
{
    if (inner.empty() || (inner & cv::Rect(0, 0, image.cols, image.rows)) != inner) {
        return false;
    }
    // I 常被 OCR 和相邻职业图标读成一行，只接受牌内居中、完整且近乎白色的一根竖笔。
    cv::Mat strokes;
    cv::cvtColor(image(inner), strokes, cv::COLOR_BGR2GRAY);
    // 归一化后，竖笔的两侧会变暗。只有笔芯从上到下都亮时，才把较暗的边缘算入笔形。
    if (threshold < 140) {
        const cv::Mat core = strokes > 140;
        const auto line = cv::boundingRect(core);
        if (line.width < 1 || line.width > 4 || line.height < 10 ||
            cv::countNonZero(core.col(line.x + line.width / 2)) != line.height) {
            return false;
        }
    }
    cv::threshold(strokes, strokes, threshold, 255, cv::THRESH_BINARY);
    const auto ink = cv::boundingRect(strokes);
    const auto color = cv::mean(image(inner), strokes);
    const auto [darkest, brightest] = std::minmax({ color[0], color[1], color[2] });
    return brightest - darkest < 25 && ink.width >= 2 && ink.width <= 4 && ink.height >= 10 && ink.height <= 15 &&
           std::abs(2 * ink.x + ink.width - inner.width) <= 3 && ink.y > 0 && ink.br().y < inner.height &&
           cv::countNonZero(strokes) >= 0.85 * ink.area();
}

static int read_panel_tier(
    const cv::Mat& image,
    const Rect& badge,
    const std::vector<TextRect>& rows,
    bool allow_dim_edges = false)
{
    static constexpr std::array<std::string_view, 6> roman { "I", "II", "III", "IV", "V", "VI" };
    int tier = 0;
    for (const auto& row : rows) {
        if (!badge.include(row.rect)) {
            continue;
        }
        const auto it = std::ranges::find(roman, row.text);
        if (it == roman.end()) {
            continue;
        }
        const int value = static_cast<int>(it - roman.begin()) + 1;
        if (row.score < 0.75 || (tier != 0 && value != tier)) {
            return 0;
        }
        tier = value;
    }
    if (tier > 1) {
        return tier;
    }
    const cv::Rect inner(badge.x + 4, badge.y + 3, badge.width - 7, badge.height - 5);
    return is_tier_one_stroke(image, inner) || (allow_dim_edges && is_tier_one_stroke(image, inner, 110)) ? 1 : 0;
}

// nullopt 表示面板未读清，空名字表示已辨认为装备或召唤物。
static std::optional<std::string> read_drag_name(const cv::Mat& image, bool check_unreadable, int& tier)
{
    OCRer tabs(image);
    tabs.set_task_info("StrongholdProtocol@DragTraitOcr");
    const auto traits = tabs.analyze();
    const bool oper_tab =
        traits && std::ranges::any_of(*traits, [](const TextRect& tr) {
            return tr.text.find("特质") != std::string::npos || tr.text.find("天赋") != std::string::npos;
        });
    if (!oper_tab && !check_unreadable) {
        return std::nullopt;
    }
    OCRer ocr(image);
    ocr.set_task_info("StrongholdProtocol@DragNameOcr");
    if (auto rows = ocr.analyze()) {
        std::string name;
        for (const auto& row : *rows) {
            const auto role = StrongholdProtocol.get_role(row.text);
            if (role == battle::Role::Drone || row.text.starts_with("“") || row.text.starts_with("「") ||
                row.text.starts_with("\"") || row.text.starts_with("『") ||
                (!BattleData.get_all_oper_names().contains(row.text) &&
                 BattleData.find_first_oper(battle::Role::Drone, row.text))) {
                return std::string {};
            }
            if (role == battle::Role::Unknown) {
                continue;
            }
            if (!name.empty() && name != row.text) {
                return {};
            }
            name = row.text;
        }
        if (oper_tab && !name.empty()) {
            tier = read_panel_tier(image, task_roi("StrongholdProtocol@Roi@DragTierFrame"), *rows, true);
            return name;
        }
    }
    return std::nullopt;
}

static int bench_stroke_tier(const cv::Mat& image, const Point& bench);

int StrongholdProtocolTaskPlugin::held_tier(const cv::Mat& preview, int panel_tier)
{
    const Rect badge = task_roi("StrongholdProtocol@Roi@DragTierFrame");
    // V 阶方牌为金黄，VI 阶为橙色。整行 OCR 常把罗马数字和精英化图标连读。
    const bool five = color_ratio(preview, badge, 21, 28, gold_lower_s, color_lower_v) > 0.08;
    const bool six = color_ratio(preview, badge, 12, 18, gold_lower_s, color_lower_v) > 0.08;
    if (five != six) {
        return six ? 6 : 5;
    }
    if (panel_tier != 0) {
        return panel_tier;
    }
    RegionOCRer numeral(preview);
    numeral.set_roi({ badge.x + 4, badge.y + 3, badge.width - 7, badge.height - 5 });
    const auto digit = numeral.analyze();
    static constexpr std::array<std::string_view, 6> roman { "I", "II", "III", "IV", "V", "VI" };
    if (digit && digit->score >= 0.9) {
        const auto it = std::ranges::find(roman, digit->text);
        if (it != roman.end()) {
            return static_cast<int>(it - roman.begin()) + 1;
        }
    }
    return 0;
}

void StrongholdProtocolTaskPlugin::deploy_opening_gifts(BoardState& board, const cv::Mat& image)
{
    std::vector<OpeningOper> opening;
    auto rest = image;
    auto slots = detect_bench_opers(rest);
    for (int attempt = 0; attempt < 2 && !need_exit(); ++attempt) {
        for (const auto& slot : slots) {
            if (need_exit() || static_cast<int>(opening.size()) == board.unread_opening_gifts) {
                break;
            }
            if (std::ranges::any_of(opening, [&](const OpeningOper& oper) {
                    return Point::distance(*oper.bench, slot) < 8;
                })) {
                continue;
            }
            const auto hover = lift_from_bench(slot);
            const auto preview = ctrler()->get_image();
            emit_input(InputEvent::Type::TOUCH_UP, hover);
            sleep(300);
            int tier = 0;
            const auto name = read_drag_name(preview, true, tier);
            if (!name || name->empty() || board.displaced_melee.contains(*name) ||
                std::ranges::any_of(board.bench_known, [&](const auto& copy) { return copy.first == *name; })) {
                continue;
            }
            tier = held_tier(preview, tier);
            if (tier == 0) {
                tier = bench_stroke_tier(rest, slot);
            }
            if (tier != 0 && tier != 2 && tier != 4) {
                continue;
            }
            opening.push_back({ *name, tier, slot, {} });
            LogInfo << "opening gift on bench" << *name << "tier" << tier << "at" << slot;
        }
        if (static_cast<int>(opening.size()) == board.unread_opening_gifts || attempt == 1) {
            break;
        }
        sleep(200);
        rest = ctrler()->get_image();
        const auto current = detect_bench_opers(rest);
        if (current != slots) {
            opening.clear(); // 方牌换位后，两次看到的可能是同一名干员。
            slots = current;
        }
    }
    if (opening.size() == 2 && slots.size() == 2 && board.round_done < 3) {
        auto& first = opening[0].tier;
        auto& second = opening[1].tier;
        // 两张赠卡的等阶分别为 II 和 IV，只能用同一批已确认的另一张补全等阶。
        if (first == second) {
            first = second = 0;
        }
        else if (first == 0) {
            first = second == 2 ? 4 : 2;
        }
        else if (second == 0) {
            second = first == 2 ? 4 : 2;
        }
    }
    board.unread_opening_gifts -= static_cast<int>(opening.size());
    for (auto& oper : opening) {
        board.bench_known.emplace_back(oper.name, oper.tier);
        board.opening_gifts.emplace_back(std::move(oper));
    }
    assign_opening_tiles(board.opening_gifts);
    if (board.opening_gifts.size() == 1 && board.occ.contains("path0") && !board.occ.contains("rear0")) {
        board.opening_gifts.front().tile = "rear0";
    }
}

std::optional<StrongholdProtocolTaskPlugin::DeployTile> StrongholdProtocolTaskPlugin::choose_tile(
    const BoardState& board,
    const std::string& name,
    int tier,
    const std::string& opening_tile,
    const cv::Mat& preview,
    std::optional<int> slots)
{
    const auto& tiles = StrongholdProtocol.get_tiles();
    const auto role = StrongholdProtocol.get_role(name);
    if ((!opening_tile.empty() && (!slots || *slots < 0 || *slots > 8)) || role == battle::Role::Unknown ||
        role == battle::Role::Drone) {
        return std::nullopt;
    }
    const bool medic = role == battle::Role::Medic;
    const auto field = medic ? field_medic_tier(board) : std::nullopt;
    if (medic && !improves_field_medic(field, tier)) {
        return std::nullopt;
    }
    std::optional<DeployTile> medic_swap;
    int medic_swap_tier = 5;
    const bool melee = opening_front_priority(name) > 0;
    const auto pick_group = [&](const std::string& group,
                                const std::vector<Point>& points) -> std::optional<DeployTile> {
        std::optional<DeployTile> swap;
        int best_value = tier;
        for (size_t i = 0; i < points.size(); ++i) {
            DeployTile tile { group + std::to_string(i), points[i] };
            if (board.unconfirmed_tiles.contains(tile.key) || (!opening_tile.empty() && tile.key != opening_tile)) {
                continue;
            }
            if (tile.key == "path0" && !melee && (opening_tile.empty() || role == battle::Role::Medic)) {
                continue;
            }
            const auto occupant = board.occ.find(tile.key);
            if (occupant == board.occ.end()) {
                if (!field && slots != 0 && is_green_tile(preview, tile.point)) {
                    return tile;
                }
                continue;
            }
            if ((!opening_tile.empty() && !medic) || board.promotion_tiles.contains(tile.key)) {
                continue;
            }
            const auto& [old_name, old_tier] = occupant->second;
            const auto old_role = StrongholdProtocol.get_role(old_name);
            if (old_tier <= 0 || old_tier >= 5 || old_role == battle::Role::Unknown ||
                old_role == battle::Role::Drone) {
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
                if (!field && (!old_melee || group != "path") && old_tier < medic_swap_tier) {
                    medic_swap_tier = old_tier;
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
            if (old_tier < best_value) {
                best_value = old_tier;
                swap = tile;
            }
        }
        return swap;
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
    return medic_swap;
}

Point StrongholdProtocolTaskPlugin::lift_from_bench(const Point& bench)
{
    // 进入拖拽预览：按下备战席小人并 hold，游戏此时把可部署格整格填绿
    // grab 机制：原地 down+hold 只会弹详情面板；须按住后快速连续 move 才抓起小人
    // 进入预览。先 down 再朝地图方向连续小步 move 激活抓取，虚影跟手后格高亮。
    emit_input(InputEvent::Type::TOUCH_DOWN, bench);
    emit_input(InputEvent::Type::WAIT_MS, bench, 350); // 先按住让游戏进入长按状态，再拖才识别为抓起
    // 控制器注入的 TOUCH_MOVE 不保证 milisec 生效（瞬时连发），游戏要看到 ｢按住后持续移动｣ 才识别为抓起；
    // 每步 MOVE 后插 WAIT_MS 让游戏帧感知位移
    const Rect probe_anchor = task_roi("StrongholdProtocol@Roi@GrabProbeTarget");
    const Point probe { bench.x + (probe_anchor.x - bench.x) / 4,
                        bench.y + (probe_anchor.y - bench.y) / 4 }; // 朝地图挪一小段
    const int grab_steps = 5;
    for (int i = 1; i <= grab_steps; ++i) {
        emit_input(
            InputEvent::Type::TOUCH_MOVE,
            Point { bench.x + (probe.x - bench.x) * i / grab_steps, bench.y + (probe.y - bench.y) * i / grab_steps });
        emit_input(InputEvent::Type::WAIT_MS, probe, 30);
    }
    // 拿起后拖到 lift 悬停点：远离全部格表格，触点不遮挡候选格的绿高亮
    const Point lp = StrongholdProtocol.get_tiles().lift;
    const int lift_ms = std::max(150, static_cast<int>(Point::distance(probe, lp)) / 2);
    for (int i = 1; i <= 8; ++i) {
        emit_input(
            InputEvent::Type::TOUCH_MOVE,
            Point { probe.x + (lp.x - probe.x) * i / 8, probe.y + (lp.y - probe.y) * i / 8 });
        emit_input(InputEvent::Type::WAIT_MS, lp, std::max(16, lift_ms / 8));
    }
    return lp;
}

int StrongholdProtocolTaskPlugin::verified_displaced_tier(
    const cv::Mat& preview,
    const std::string& expected,
    bool for_sale)
{
    const Rect badge = task_roi("StrongholdProtocol@Roi@DragTierFrame");
    if (preview.empty() || !Rect(0, 0, preview.cols, preview.rows).include(badge) ||
        color_ratio(preview, badge, gold_lower_h, gold_upper_h, gold_lower_s, color_lower_v) > 0.08) {
        return 0;
    }
    if (for_sale) {
        OCRer sell(preview, task_roi("StrongholdProtocol@Roi@ShopSell"));
        sell.set_required({ "拖拽至此售出" });
        if (!sell.analyze()) {
            return 0;
        }
    }
    OCRer tabs(preview);
    tabs.set_task_info("StrongholdProtocol@DragTraitOcr");
    const auto traits = tabs.analyze();
    if (!traits || !std::ranges::any_of(*traits, [](const auto& t) { return t.text == "特质" || t.text == "天赋"; })) {
        return 0;
    }
    OCRer panel(preview);
    panel.set_task_info("StrongholdProtocol@DragNameOcr");
    const auto rows = panel.analyze();
    if (!rows) {
        return 0;
    }
    bool named = false;
    for (const auto& row : *rows) {
        if (row.rect.y + row.rect.height / 2 < badge.y) {
            // 名字拆行或只读到别名的一部分时，不能确认手上的干员。
            if (named || row.text != expected || row.score < 0.9) {
                return 0;
            }
            named = true;
        }
    }
    const int tier = read_panel_tier(preview, badge, *rows, !for_sale);
    return named && tier < 5 ? tier : 0;
}

static std::vector<cv::Rect> detect_bench_badges(const cv::Mat& image);

static int bench_stroke_tier(const cv::Mat& image, const Point& bench)
{
    const Rect press = task_roi("StrongholdProtocol@Roi@BenchPressOffset");
    const auto badges = detect_bench_badges(image);
    for (const auto& badge : badges) {
        // 差分方块和整块方牌的左上角会相差几像素，不能借用相邻干员的方牌。
        if (Point::distance(bench, Point { badge.x + press.x, badge.y + press.y }) <= 6) {
            const cv::Rect inner(badge.x + 4, badge.y + 4, badge.width - 8, badge.height - 8);
            if (is_tier_one_stroke(image, inner)) {
                return 1;
            }
            // II 和 III 由同高、等间距的完整竖笔组成。缺笔、斜笔或贴边的数字不能确认等阶。
            cv::Mat mask;
            cv::cvtColor(image(inner), mask, cv::COLOR_BGR2GRAY);
            cv::threshold(mask, mask, 140, 255, cv::THRESH_BINARY);
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            if (contours.size() != 2 && contours.size() != 3) {
                return 0;
            }
            std::vector<cv::Rect> strokes;
            for (const auto& contour : contours) {
                const auto line = cv::boundingRect(contour);
                if (line.width < 2 || line.width > 4 || line.height < 10 || line.height > 15 || line.y <= 0 ||
                    line.br().y >= inner.height || cv::countNonZero(mask(line)) < 0.85 * line.area()) {
                    return 0;
                }
                strokes.emplace_back(line);
            }
            std::ranges::sort(strokes, {}, &cv::Rect::x);
            const auto& first = strokes.front();
            for (size_t i = 1; i < strokes.size(); ++i) {
                const auto& line = strokes[i];
                const int pitch = line.x - strokes[i - 1].x;
                if (pitch < 5 || pitch > 7 || std::abs(line.y - first.y) > 1 ||
                    std::abs(line.br().y - first.br().y) > 1) {
                    return 0;
                }
            }
            const auto ink = first | strokes.back();
            return ink.x > 0 && ink.br().x < inner.width && std::abs(2 * ink.x + ink.width - inner.width) <= 2
                       ? static_cast<int>(strokes.size())
                       : 0;
        }
    }
    return 0;
}

StrongholdProtocolTaskPlugin::Placement StrongholdProtocolTaskPlugin::place_held(const Point& hover, const Point& tile)
{
    const int dist = static_cast<int>(Point::distance(hover, tile));
    drop_held(hover, tile, std::max(300, dist), 150);
    for (int i = 0; i < 10 && !need_exit(); ++i) {
        sleep(200);
        OCRer facing(ctrler()->get_image());
        facing.set_task_info("StrongholdProtocol@FacingCancelOcr");
        if (auto r = facing.analyze(); r && !r->empty()) {
            m_last_tile = tile;
            return set_facing(tile) ? Placement::Confirmed : Placement::FacingPending;
        }
    }
    save_img(utils::path("debug") / utils::path("stronghold"), false);
    return Placement::Unconfirmed;
}

std::optional<StrongholdProtocolTaskPlugin::DeployTile>
    StrongholdProtocolTaskPlugin::empty_melee_tile(const BoardState& board, const cv::Mat& preview)
{
    const auto& tiles = StrongholdProtocol.get_tiles();
    for (const auto& [group, points] :
         { std::pair { "path", &tiles.path }, { "rear", &tiles.rear }, { "safe", &tiles.safe } }) {
        for (size_t i = 0; i < points->size(); ++i) {
            const auto key = group + std::to_string(i);
            if (!board.occ.contains(key) && !board.promotion_tiles.contains(key) &&
                !board.unconfirmed_tiles.contains(key) && is_green_tile(preview, (*points)[i])) {
                return DeployTile { key, (*points)[i] };
            }
        }
    }
    return std::nullopt;
}

bool StrongholdProtocolTaskPlugin::place_displaced_melee(
    BoardState& board,
    const DeployTile& tile,
    const Point& hover,
    const std::string& name,
    int tier,
    int slots)
{
    const auto placement = place_held(hover, tile.point);
    if (placement == Placement::Unconfirmed) {
        LogWarn << "displaced operator placement unconfirmed" << name << tile.key;
        return false;
    }
    // 朝向和名额都确认后再记占位。若发生意外交换，不继续处置身份未明的备战席干员。
    if (placement != Placement::Confirmed || slots_left(ctrler()->get_image()) != slots - 1) {
        board.pending_deploy = true;
        LogWarn << "displaced operator facing or capacity unconfirmed" << name << tile.key;
        return false;
    }
    board.occ[tile.key] = { name, tier };
    board.displaced_melee.erase(board.displaced_melee.find(name));
    LogInfo << "redeployed displaced melee" << name << "tier" << tier << "to" << tile.key;
    return true;
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
    const auto before_deploy = ctrler()->get_image();
    const auto slots0 = slots_left(before_deploy);
    const auto& tiles = StrongholdProtocol.get_tiles();
    if (slots0 == 0 && board.promotion_tiles.empty() && board.displaced_melee.empty() &&
        board.occ.size() == tiles.path.size() + tiles.safe.size() + tiles.rear.size() &&
        std::ranges::all_of(board.occ, [](const auto& entry) { return entry.second.second >= 5; })) {
        LogTrace << "bench deployment skipped: all tiles occupied by high-tier operators";
        return std::nullopt;
    }
    const Point lp = lift_from_bench(bench);
    const auto preview = ctrler()->get_image();

    int observed_tier = 0;
    const auto panel_name = read_drag_name(preview, observed != nullptr, observed_tier);
    if (observed) {
        *observed = panel_name;
    }
    const auto name = panel_name.value_or("");
    if (name.empty() || (!name_hint.empty() && name != name_hint)) {
        emit_input(InputEvent::Type::TOUCH_UP, lp);
        sleep(300);
        LogInfo << "bench operator not confirmed, expected" << name_hint << "read" << name;
        return std::nullopt;
    }
    auto known = std::ranges::find(board.bench_known, name, &std::pair<std::string, int>::first);
    if (name_hint.empty() && board.displaced_melee.contains(name)) {
        // 换下者可能与其他副本同名。只复核本次手持的近战，且重扫也不能触发第二次换人。
        if (!medic_only && known == board.bench_known.end() && slots0 && *slots0 > 0 && *slots0 <= 8 &&
            BattleData.get_all_oper_names().contains(name) && !BattleData.find_first_oper(battle::Role::Drone, name) &&
            !std::ranges::any_of(board.occ, [&](const auto& entry) {
                return entry.second.first == name && entry.second.second >= 5;
            })) {
            if (const auto empty = empty_melee_tile(board, preview)) {
                const int low_tier = verified_displaced_tier(preview, name, false);
                if (low_tier > 0 && !need_exit()) {
                    return place_displaced_melee(board, *empty, lp, name, low_tier, *slots0)
                               ? std::optional { empty->point }
                               : std::nullopt;
                }
            }
        }
        emit_input(InputEvent::Type::TOUCH_UP, lp);
        return std::nullopt;
    }
    if (known != board.bench_known.end()) {
        // 同名方牌换位后无法按购买顺序区分。清除互相矛盾的旧等阶，当次拿起的干员单独核验。
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
    int tier = observed_tier > 0 ? observed_tier : tier_hint;
    if (tier_hint >= 5) {
        tier = std::max(tier, tier_hint);
    }
    if (tier == 0) {
        tier = bench_stroke_tier(before_deploy, bench);
    }
    // 面板黄框色判 V/VI（判定区 Roi@DragTierFrame，金框占比 > 0.08）
    if (color_ratio(
            preview,
            task_roi("StrongholdProtocol@Roi@DragTierFrame"),
            gold_lower_h,
            gold_upper_h,
            gold_lower_s,
            color_lower_v) > 0.08) {
        tier = std::max(tier, 5);
    }
    if (!board.promotion_tiles.empty() && slots0 && *slots0 > 0) {
        for (const auto& [group, points] :
             { std::pair { "path", &tiles.path }, { "rear", &tiles.rear }, { "safe", &tiles.safe } }) {
            for (size_t i = 0; i < points->size(); ++i) {
                const auto key = group + std::to_string(i);
                if (board.promotion_tiles.contains(key) && is_green_tile(preview, (*points)[i])) {
                    LogInfo << "promotion freed tile" << key;
                    board.occ.erase(key);
                    board.promotion_tiles.erase(key);
                }
            }
        }
    }
    // 尚未辨认的赠卡留给开局排位。已知的买入干员和换下者仍可按原流程部署。
    const bool awaiting_opening =
        name_hint.empty() &&
        ((board.unread_opening_gifts > 0 && known == board.bench_known.end()) ||
         std::ranges::any_of(board.opening_gifts, [&](const auto& gift) { return gift.name == name; }));
    if (awaiting_opening || (medic_only && !is_medic(name))) {
        emit_input(InputEvent::Type::TOUCH_UP, lp);
        return std::nullopt;
    }
    const auto target = choose_tile(board, name, tier, opening_tile, preview, slots0);
    if (!target) {
        if (is_medic(name)) {
            if (known == board.bench_known.end()) {
                board.bench_known.emplace_back(name, tier);
            }
            else {
                known->second = tier;
            }
        }
        emit_input(InputEvent::Type::WAIT_MS, lp, 300);
        emit_input(InputEvent::Type::TOUCH_UP, lp);
        LogInfo << "no free tile, returned to bench:" << name;
        return std::nullopt;
    }
    const auto& [target_key, target_pt] = *target;
    std::pair<std::string, int> swap_old;
    if (const auto it = board.occ.find(target_key); it != board.occ.end()) {
        swap_old = it->second;
    }
    LogInfo << "deploy target" << target_key << target_pt.x << "," << target_pt.y;

    const auto placement = place_held(lp, target_pt);
    const auto after_deploy = ctrler()->get_image();
    const auto slots1 = slots_left(after_deploy);
    if (!opening_tile.empty() && (placement != Placement::Confirmed || !slots0 || !slots1 || *slots1 < 0 ||
                                  (*slots1 != *slots0 && *slots1 != *slots0 - 1))) {
        board.unconfirmed_tiles[target_key] = name;
        board.pending_deploy = true;
        LogWarn << "opening placement unconfirmed" << name << target_key;
        return std::nullopt;
    }
    if (placement == Placement::Unconfirmed) {
        LogWarn << "deploy failed (probably equipment or occupied tile):" << name;
        return std::nullopt;
    }

    const bool is_swap = !swap_old.first.empty() && !(slots0 && slots1 && *slots1 < *slots0);
    board.promotion_tiles.erase(target_key);
    board.occ[target_key] = { name, tier };
    if (known != board.bench_known.end()) {
        board.bench_known.erase(known);
    }
    // 意外交换检测：非换人放置但空位数没减少，说明目标格原本有未记录的干员，身份未知不卖，回合末重扫部署
    if (!is_swap && slots0 && slots1 && *slots1 >= *slots0) {
        board.pending_deploy = true;
        LogWarn << "unexpected swap at" << target_key << ", displaced occupant left for round-end scan";
        return target_pt;
    }

    if (is_swap) {
        board.pending_deploy = true;
        const auto& [old_name, old_tier] = swap_old;
        const bool low_melee = old_tier > 0 && old_tier < 5 && opening_front_priority(old_name) > 0;
        if (low_melee) {
            board.displaced_melee.emplace(old_name);
        }
        const bool ambiguous =
            name == old_name ||
            std::ranges::any_of(board.bench_known, [&](const auto& oper) { return oper.first == old_name; }) ||
            std::ranges::any_of(board.occ, [&](const auto& entry) {
                return entry.second.first == old_name && entry.second.second >= 5;
            });
        const bool may_redeploy = low_melee && slots1 && *slots1 > 0;
        const bool may_sell = StrongholdProtocol.priority(old_name) == 0;
        if (placement == Placement::Confirmed && slots0 && *slots0 <= 8 && slots0 == slots1 &&
            !is_green_tile(preview, target_pt) && old_tier > 0 && old_tier < 5 && !ambiguous &&
            (may_redeploy || may_sell) && BattleData.get_all_oper_names().contains(old_name) &&
            !BattleData.find_first_oper(battle::Role::Drone, old_name)) {
            const auto& baseline = before_purchase.empty() ? before_deploy : before_purchase;
            const auto out = new_bench_slot(baseline, after_deploy);
            // 原位换人时方牌只变了数字。差分没命中或抓到别的物体，都可再核对刚腾出的原位。
            std::vector<Point> candidates { out.value_or(bench) };
            if (out && *out != bench) {
                candidates.emplace_back(bench);
            }
            auto gold0 = may_redeploy ? std::optional<int> {} : read_gold(after_deploy);
            for (const auto& candidate : candidates) {
                if ((!may_redeploy && !gold0) || need_exit()) {
                    break;
                }
                const auto hover = lift_from_bench(candidate);
                bool dropped = false;
                OnScopeLeave([&] {
                    if (!dropped) {
                        emit_input(InputEvent::Type::TOUCH_UP, hover);
                        sleep(800);
                    }
                });
                const auto displaced = ctrler()->get_image();
                const auto empty = may_redeploy ? empty_melee_tile(board, displaced) : std::nullopt;
                if (!empty && !may_sell) {
                    break;
                }
                const int verified_tier = verified_displaced_tier(displaced, old_name, !empty);
                if (verified_tier == 0 || need_exit()) {
                    LogInfo << "unverified displaced operator returned:" << old_name << candidate;
                    continue;
                }
                if (empty) {
                    dropped = true;
                    place_displaced_melee(board, *empty, hover, old_name, verified_tier, *slots1);
                    break;
                }
                if (!gold0) {
                    gold0 = read_gold(after_deploy);
                    if (!gold0 || need_exit()) {
                        break;
                    }
                }
                const Rect sell = task_roi("StrongholdProtocol@Roi@ShopSell");
                drop_held(hover, { sell.x + sell.width / 2, sell.y + sell.height / 2 }, 250, 200);
                dropped = true;
                sleep(1000);
                const auto gold1 = read_gold(ctrler()->get_image());
                if (gold1 && *gold1 > *gold0) {
                    if (low_melee) {
                        board.displaced_melee.erase(board.displaced_melee.find(old_name));
                    }
                    if (auto it = board.purchases.find(old_name);
                        it != board.purchases.end() && it->second.unmerged > 0) {
                        --it->second.unmerged;
                    }
                    LogInfo << "sold swapped-out operator" << old_name << "gold" << *gold0 << "->" << *gold1;
                }
                else {
                    LogWarn << "sale unconfirmed, bench will be rescanned:" << old_name;
                }
                // 已在出售区松手，即使资金读数缺失也不能再试，重扫时重新辨认席上的干员。
                break;
            }
        }
    }
    LogInfo << "deployed" << (name.empty() ? "?" : name) << "tier" << tier << "to" << target_key << target_pt.x << ","
            << target_pt.y;
    return target_pt;
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
    auto& roster = board.medic_bench;
    if (!std::ranges::equal(slots, roster, [](const Point& slot, const auto& entry) {
            return Point::distance(slot, entry.first) < 8;
        })) {
        roster.clear();
        for (const auto& slot : slots) {
            roster.emplace_back(slot, std::nullopt);
        }
    }
    bool inspected = false;
    for (auto& [slot, name] : roster) {
        if (need_exit()) {
            return inspected;
        }
        if (name && (!is_medic(*name) || (field && board.promotion_tiles.empty() &&
                                          !std::ranges::any_of(board.bench_known, [&](const auto& copy) {
                                              return copy.first == *name && improves_field_medic(field, copy.second);
                                          })))) {
            continue;
        }
        inspected = true;
        if (deploy_from_bench(board, slot, {}, 0, {}, {}, &name, true)) {
            roster.clear();
            return true;
        }
    }
    // 名单完整且方牌位置稳定时，才清除已离开备战席的医疗记录。
    if (known_medic && std::ranges::all_of(roster, [](const auto& entry) { return entry.second.has_value(); })) {
        const auto missing = [&](const auto& copy) {
            return is_medic(copy.first) &&
                   std::ranges::none_of(roster, [&](const auto& entry) { return entry.second == copy.first; });
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
    // 备战席小人按头顶等阶方牌检测定位（台面布局随人数变化，固定扫点不可靠）：
    // 每轮重新检测当前剩余人，逐个尝试部署，部署成功小人消失故循环重扫
    for (int round = 0; round < 10 && !need_exit(); ++round) {
        const auto opers = detect_bench_opers(ctrler()->get_image());
        if (opers.empty()) {
            if (!board.displaced_melee.empty()) {
                sleep(200);
                if (!detect_bench_opers(ctrler()->get_image()).empty()) {
                    return true;
                }
                board.displaced_melee.clear();
            }
            return false;
        }
        auto unseen_displaced = board.displaced_melee;
        auto unseen = board.bench_known;
        std::erase_if(unseen, [](const auto& copy) {
            const auto role = StrongholdProtocol.get_role(copy.first);
            return role == battle::Role::Medic || role == battle::Role::Drone || role == battle::Role::Unknown;
        });
        bool unreadable = false;
        bool placed = false;
        for (const auto& pt : opers) {
            std::optional<std::string> name = std::string {}; // 满员且全为高阶时会在拿起前返回。
            if (deploy_from_bench(
                    board,
                    pt,
                    {},
                    0,
                    {},
                    {},
                    unseen.empty() && unseen_displaced.empty() ? nullptr : &name)) {
                board.medic_bench.clear();
                placed = true;
                break;
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
        if (!placed) {
            // 同名份数分别核销，已核对但无格可放的干员不需要因旁边的方牌漏读而重扫。
            if (!unreadable && !unseen_displaced.empty() && detect_bench_opers(ctrler()->get_image()) == opers) {
                for (const auto& missing : unseen_displaced) {
                    board.displaced_melee.erase(board.displaced_melee.find(missing));
                }
            }
            return !board.displaced_melee.empty() || (unreadable && !unseen.empty());
        }
    }
    return !board.displaced_melee.empty() || !board.bench_known.empty();
}

bool StrongholdProtocolTaskPlugin::set_facing(const Point& tile)
{
    // 朝向拖拽：向右拖一段距离后松手；轮询 ｢点击取消｣ 消失即生效（≤2s/次）
    for (int attempt = 0; attempt < 3; ++attempt) {
        const int dist = task_roi("StrongholdProtocol@Roi@FacingDragDist").width + 40 * attempt;
        drag_to(tile, Point(tile.x + dist, tile.y), 300, 200);
        for (int i = 0; i < 10 && !need_exit(); ++i) {
            sleep(200);
            OCRer facing(ctrler()->get_image());
            facing.set_task_info("StrongholdProtocol@FacingCancelOcr");
            if (auto r = facing.analyze(); !r || r->empty()) {
                return true;
            }
        }
        LogWarn << "facing not applied, retry" << attempt;
    }
    LogError << "facing failed after retries";
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
    if (scan.empty()) {
        return std::nullopt;
    }
    // 深色方牌出现时背景变暗，离开时背景变亮。
    cv::Mat before_gray, after_gray, mask;
    cv::cvtColor(before(scan), before_gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(after(scan), after_gray, cv::COLOR_BGR2GRAY);
    cv::subtract(before_gray, after_gray, mask);
    cv::threshold(mask, mask, 40, 255, cv::THRESH_BINARY);
    // 等阶数字的笔画会在方牌内留下差分空洞。
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, { 10, 10 }));

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
    if (square.width >= 20) {
        const Rect press = task_roi("StrongholdProtocol@Roi@BenchPressOffset");
        const Point pt(scan.x + square.x + press.x, scan.y + square.y + press.y);
        LogTrace << "new bench slot square" << scan.x + square.x << scan.y + square.y << "side" << square.width
                 << "press" << pt.x << pt.y;
        return pt;
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

// 检测休整界面备战席上的小人：每个小人头顶一块深色圆角等阶方牌（内有浅色罗马数字）。
// 扫描范围由 Roi@BenchScan 限定，布局随人数与地图变化故按方牌形态检测而非固定坐标。
static std::vector<cv::Rect> detect_bench_badges(const cv::Mat& image)
{
    const Rect scan = task_roi("StrongholdProtocol@Roi@BenchScan");
    const cv::Rect scan_cv = make_rect<cv::Rect>(scan) & cv::Rect(0, 0, image.cols, image.rows);
    if (scan_cv.empty()) {
        return {};
    }
    const cv::Mat sub = image(scan_cv);

    // 等阶方牌是深灰圆角方块（面 ~30，远低于木台面 ~70）：低亮度粗筛候选块
    cv::Mat gray;
    cv::cvtColor(sub, gray, cv::COLOR_BGR2GRAY);
    cv::Mat dark;
    cv::threshold(gray, dark, 60, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(dark, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Rect> badges;
    for (const auto& c : contours) {
        const cv::Rect rc = cv::boundingRect(c);
        // 方牌约 18-34px 宽、15-34px 高，横向矩形（实测 25x24）
        if (rc.width < 18 || rc.width > 34 || rc.height < 15 || rc.height > 34) {
            continue;
        }
        const double ar = static_cast<double>(rc.width) / rc.height;
        if (ar < 0.8 || ar > 1.8) {
            continue;
        }
        // 彩色数字会抬高牌面均值。用外轮廓包围的面积区分圆角方牌和台面接缝。
        if (cv::contourArea(c) < rc.area() * 0.75) {
            continue;
        }
        // 避开彩色边框，只检查牌内的数字笔画。
        const cv::Rect inner(rc.x + 4, rc.y + 4, rc.width - 8, rc.height - 8);
        cv::Mat strokes;
        cv::threshold(gray(inner), strokes, 140, 255, cv::THRESH_BINARY);
        if (cv::countNonZero(strokes) < inner.area() * 0.03) {
            continue;
        }
        badges.emplace_back(rc + scan_cv.tl());
    }
    return badges;
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
                if (title == "模拟完成" || title == "模拟中断") {
                    outcome = title == "模拟完成";
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
