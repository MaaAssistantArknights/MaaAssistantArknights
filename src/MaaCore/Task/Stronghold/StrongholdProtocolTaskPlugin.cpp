#include "StrongholdProtocolTaskPlugin.h"

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

#include <algorithm>
#include <chrono>
#include <climits>

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
        switch (scene) {
        case Scene::Navigate:
            // 活动主界面：走 @Nav 链导航到策略选择页（链停在策略页，不代选策略）
            ProcessTask(*this, { "StrongholdProtocol@Nav" }).run();
            break;
        case Scene::StrategySelect:
            if (!pick_strategy()) {
                LogWarn << "no eligible strategy found, task finished";
                return true;
            }
            progressed = true;
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
            progressed = board.round_done != round_before;
            break;
        }
        case Scene::ConfirmPopup:
            handle_popups();
            break;
        case Scene::Settlement:
            progressed = handle_settlement(board);
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

StrongholdProtocolTaskPlugin::Scene
    StrongholdProtocolTaskPlugin::detect_scene(const cv::Mat& image, UnknownScreen& screen)
{
    auto match = [&image](const std::string& task_name) {
        Matcher matcher(image);
        matcher.set_task_info(task_name);
        return matcher.analyze().has_value();
    };
    auto ocr_contains = [&image](const std::string& task_name) {
        // 场景判定必须用多结果 OCRer：roi 内常有多行文本，RegionOCRer 单结果
        // 模式可能取到别的行导致关键词漏判
        OCRer ocr(image);
        ocr.set_task_info(task_name);
        return ocr.analyze().has_value();
    };

    if (match("StrongholdProtocol@ReturnHome")) {
        return Scene::Settlement;
    }
    // 悬赏判定和未知场景共用同一份全屏文字，保留各行原文供词表匹配。
    OCRer ocr(image);
    ocr.set_task_info("StrongholdProtocol@ScreenTextOcr");
    const auto text = ocr.analyze().value_or(std::vector<TextRect> {});
    if (is_bounty_select(text)) {
        return Scene::BountySelect;
    }
    // 自选编队页：CustomOpersConfirm 模板命中即编队页（准备就绪按钮常亮可点，选人并非前置条件）
    if (match("StrongholdProtocol@CustomOpersConfirm")) {
        // ｢确认本局信息｣ 页与编队页共用同款 ｢准备就绪｣ 按钮，模板分不开，按页面文字区分
        return ocr_contains("StrongholdProtocol@ConfirmInfo") ? Scene::ConfirmInfo : Scene::CustomSquad;
    }
    // 策略网格页（歌利亚环节）：Goliath 模板或 SPR/SPA 确认按钮任一命中；
    // OCR ｢选择策略｣ 标题与实际画面不符，不用其判定
    if (match("StrongholdProtocol@Goliath") || match("StrongholdProtocol@SPRConfirmSelect") ||
        match("StrongholdProtocol@SPAConfirmSelect")) {
        return Scene::StrategySelect;
    }
    if (ocr_contains("StrongholdProtocol@ConfirmReadyOcr")) {
        return Scene::ConfirmPopup; // 弹窗会遮挡准备按钮，必须优先于 RoundPrepare 处理
    }
    if (ocr_contains("StrongholdProtocol@ReadyButton")) {
        return Scene::RoundPrepare;
    }
    // 活动主界面（极速模拟入口）放最后：对局内各场景优先
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
    screen.in_battle = std::ranges::any_of(keywords("StrongholdProtocol@Keywords@Battle"), contains);
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

bool StrongholdProtocolTaskPlugin::is_bounty_select(const std::vector<TextRect>& text)
{
    const auto task = Task.get<OcrTaskInfo>("StrongholdProtocol@BountyTitle");
    if (!task || text.size() < 2) {
        return false;
    }
    const auto& config = OcrConfig::get_instance();
    auto keywords = task->text;
    for (auto& keyword : keywords) {
        keyword = config.process_equivalence_class(keyword);
    }
    int matches = 0;
    for (const auto& row : text) {
        if (!task->roi.empty() && !task->roi.include(row.rect)) {
            continue;
        }
        const auto line = config.process_equivalence_class(row.text);
        const bool matched = std::ranges::any_of(keywords, [&](const std::string& keyword) {
            return task->full_match ? line == keyword : line.find(keyword) != std::string::npos;
        });
        if (matched && ++matches == 2) {
            return true; // 标题和一张卡，或两张卡
        }
    }
    return false;
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
        ctrler()->click(rect);
        return true;
    }
    return false;
}

// ---------------- 干员判定（权重表） ----------------

bool StrongholdProtocolTaskPlugin::name_match(const std::string& name, const std::string& key)
{
    // 干员名全等匹配；OCR 错字由 CharsNameOcrReplace 纠错表在识别层规整
    return !name.empty() && name == key;
}

int StrongholdProtocolTaskPlugin::oper_priority(const std::string& name)
{
    const auto& priority = StrongholdProtocol.get_priority();
    for (size_t i = 0; i < priority.size(); ++i) {
        if (name_match(name, priority[i])) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool StrongholdProtocolTaskPlugin::is_safe_oper(const std::string& name)
{
    return std::ranges::any_of(StrongholdProtocol.get_safe_keys(), [&](const std::string& key) {
        return name_match(name, key);
    });
}

bool StrongholdProtocolTaskPlugin::is_healer(const std::string& name)
{
    return std::ranges::any_of(StrongholdProtocol.get_healers(), [&](const std::string& h) {
        return name_match(name, h);
    });
}

bool StrongholdProtocolTaskPlugin::is_dps_oper(const std::string& name)
{
    // 空名（OCR 未读出）时 d.find("") 恒为 0，须先排除
    if (name.empty()) {
        return false;
    }
    return std::ranges::any_of(StrongholdProtocol.get_dps(), [&](const std::string& d) { return name_match(name, d); });
}

int StrongholdProtocolTaskPlugin::oper_rank(const std::string& name, int tier)
{
    // 干员价值：优先干员 > 其他 V/VI 输出 > 其他 V/VI > 低星
    int p = oper_priority(name);
    if (p >= 0) {
        return 1000 - p;
    }
    if (tier >= 5) {
        return 500 + (is_dps_oper(name) ? 50 : 0);
    }
    return tier;
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

bool StrongholdProtocolTaskPlugin::pick_strategy()
{
    LogTraceFunction;

    // 点击任意网格让布局左移到固定布局（选策略界面有倒计时，节奏尽量收紧）
    ProcessTask(*this, { "StrongholdProtocol@StrategyGridProbe" }).run();

    // 策略格识别：列心恒定（滚动只改变行 y），行由证据聚类得出，不依赖固定网格布局。
    // 证据两类：素描立绘/头像的亮像素块；卡角标记块（NEW/金章/彩章/红三角按色相段检出，
    // 按各自相对卡心的偏移归算）。行内证据吸附列心、行 y 取中位即得卡心。暗立绘卡可能
    // 漏检，靠滚动翻页覆盖。
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
    std::vector<std::string> tried;
    for (int page = 0; page < 8 && !need_exit(); ++page) {
        const auto image = ctrler()->get_image();
        const auto cells = detect_grid_cells(image);
        if (cells.empty()) {
            // 一格都找不到是识别异常（区别于全达标），存图留证
            save_img(utils::path("debug") / utils::path("stronghold"));
            LogWarn << "strategy grid not detected, page" << page;
        }
        for (const auto& pt : cells) {
            // 已通过（角标）：该格右上区有金/青色块 → 跳过
            if (has_color_badge(image, pt.x, pt.y) || (m_target <= 3 && has_gold_badge(image, pt.x, pt.y))) {
                continue;
            }

            ctrler()->click(pt);
            sleep(400);

            // 右侧面板读取：策略名 + 已通过次数；面板有切换动画，读不到名字重试
            std::string strategy_name;
            int passed = 0;
            for (int attempt = 0; attempt < 3; ++attempt) {
                OCRer ocr(ctrler()->get_image());
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

            LogInfo << "select strategy:" << strategy_name << "passed" << passed << "target" << m_target;
            json::value info = basic_info_with_what("StrongholdStrategy");
            info["details"]["name"] = strategy_name;
            info["details"]["passed"] = passed;
            info["details"]["target"] = m_target;
            callback(AsstMsg::SubTaskExtraInfo, info);
            m_current_strategy = strategy_name;
            // 直接点确认按钮（click_template 单击即返回，不走 nav 链避免被 LoopWaitStart 挂住）
            click_template("StrongholdProtocol@SPRConfirmSelect");
            // 确认生效则离开策略界面（Goliath 模板消失）；仍在界面说明点击无效，
            // 靠 tried 记录跳过该策略继续遍历，避免对同一确认按钮死磕到倒计时耗尽
            sleep(1500);
            Matcher still_here(ctrler()->get_image());
            still_here.set_task_info("StrongholdProtocol@Goliath");
            if (!still_here.analyze()) {
                return true;
            }
            LogWarn << "strategy confirm had no effect, skip:" << strategy_name;
            m_current_strategy.clear();
            continue;
        }
        // 本页无可用策略：上滚一步继续查找（extra 水平补偿段消除竖直松手惯性）
        ProcessTask(*this, { "StrongholdProtocol@StrategyListScroll" }).run();
        const auto after = ctrler()->get_image();
        const auto shift = measure_grid_shift(image, after);
        if (shift && std::abs(*shift) < 13) {
            break; // 列表不再移动 = 已到底
        }
    }
    return false;
}

bool StrongholdProtocolTaskPlugin::pick_squad(BoardState& board)
{
    LogTraceFunction;

    // 已选过人但页面没推进：只补点确认，不再点名字（再点会取消已选干员）
    if (board.squad_picked) {
        click_template("StrongholdProtocol@CustomOpersConfirm");
        return true;
    }

    // 自选编队按权重每等阶选 2 名；编队为空（识别不到可选干员）时直接确认开始
    OCRer ocr(ctrler()->get_image());
    ocr.set_task_info("StrongholdProtocol@SquadOpers");
    auto results_opt = ocr.analyze();

    std::vector<TextRect> names;
    std::vector<Rect> banned_marks; // ｢本局禁用｣ 标记位置，被禁干员名字紧邻其上方
    if (results_opt) {
        for (const auto& tr : *results_opt) {
            if (tr.text.find("禁用") != std::string::npos) {
                banned_marks.emplace_back(tr.rect);
                continue;
            }
            // 只选名单内干员（权重表全等命中）；被 ban 的名牌有遮挡读不全，
            // 名单不中自然出局，界面说明文字与 OCR 噪声同理
            if (oper_priority(tr.text) < 0) {
                continue;
            }
            names.emplace_back(tr);
        }
    }
    auto is_banned = [&](const TextRect& tr) {
        const int cx = tr.rect.x + tr.rect.width / 2;
        return std::ranges::any_of(banned_marks, [&](const Rect& mark) {
            return std::abs(cx - (mark.x + mark.width / 2)) < 60 && mark.y > tr.rect.y && mark.y - tr.rect.y < 80;
        });
    };

    // 按 y 聚类成行：相邻名字 y 差 < 80px 归一行（上排 V、下排 VI）
    std::ranges::sort(names, [](const TextRect& a, const TextRect& b) { return a.rect.y < b.rect.y; });
    std::vector<std::vector<TextRect>> rows;
    for (const auto& tr : names) {
        if (rows.empty() || tr.rect.y - rows.back().front().rect.y > 80) {
            rows.emplace_back();
        }
        rows.back().emplace_back(tr);
    }

    std::vector<std::string> picked;
    for (auto& row : rows) {
        std::ranges::sort(row, [](const TextRect& a, const TextRect& b) {
            int pa = oper_priority(a.text), pb = oper_priority(b.text);
            return (pa < 0 ? 999 : pa) < (pb < 0 ? 999 : pb);
        });
        int chosen = 0;
        for (const auto& tr : row) {
            if (chosen >= 2 || is_banned(tr)) {
                continue;
            }
            // OCR 会把同一干员读出重复条目（名字与名牌双行），重复点击会 toggle 取消已选
            if (std::ranges::find(picked, tr.text) != picked.end()) {
                continue;
            }
            ctrler()->click(Point(
                tr.rect.x + tr.rect.width / 2,
                tr.rect.y + tr.rect.height / 2 + task_roi("StrongholdProtocol@Roi@SquadNameOffset").y));
            sleep(400);
            picked.emplace_back(tr.text);
            ++chosen;
        }
    }
    LogInfo << "custom squad picked:" << picked.size() << "operators";
    board.squad_picked = true;
    click_template("StrongholdProtocol@CustomOpersConfirm");
    return true;
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

StrongholdProtocolTaskPlugin::ShopInfo StrongholdProtocolTaskPlugin::read_shop(const cv::Mat& image)
{
    ShopInfo info;
    info.gold = read_gold(image);
    info.round = read_number(image, "StrongholdProtocol@ShopRoundOcr");

    // 升级按钮随商店等级移动：取 ｢升级｣/｢满级｣ 文字中心 x 定位，命中 ｢满级｣ 即商店已满
    OCRer bottom(image);
    bottom.set_task_info("StrongholdProtocol@ShopBottomOcr");
    if (auto results = bottom.analyze(); results && !results->empty()) {
        const auto& button = results->front();
        info.upgrade_x = button.rect.x + button.rect.width / 2;
        for (const auto& tr : *results) {
            if (tr.text.find("满级") != std::string::npos) {
                info.maxed = true;
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
    StrongholdProtocolTaskPlugin::read_shop_cards(const cv::Mat& image) const
{
    OCRer ocr(image);
    ocr.set_task_info("StrongholdProtocol@ShopCardsOcr");
    auto results_opt = ocr.analyze();

    const auto is_oper_name = [](const std::string& text) {
        return oper_priority(text) >= 0 || is_dps_oper(text) || is_healer(text) || is_safe_oper(text);
    };
    const auto tier_roi = task_roi("StrongholdProtocol@Roi@ShopTierY");
    const auto cost_roi = task_roi("StrongholdProtocol@Roi@ShopCostY");
    auto card_xs = results_opt ? detect_shop_card_x(*results_opt, cost_roi, tier_roi) : std::vector<int> {};
    // 按钮右侧一段内的文字（按钮费用等）不算卡牌（按钮中心右侧 150px@1080p 之外才算）
    if (m_upgrade_x) {
        const int span = task_roi("StrongholdProtocol@Roi@ShopUpgradeButtonSpan").width;
        std::erase_if(card_xs, [&](int x) { return x <= *m_upgrade_x + span; });
    }

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
    const int tier_lo = tier_roi.y, tier_hi = tier_roi.y + tier_roi.height;
    const int cost_lo = cost_roi.y, cost_hi = cost_roi.y + cost_roi.height;
    const int empty_lo = empty_roi.y, empty_hi = empty_roi.y + empty_roi.height;
    const int pact_lo = pact_roi.y, pact_hi = pact_roi.y + pact_roi.height;
    const int name_lo2 = name_roi2.y, name_hi2 = name_roi2.y + name_roi2.height;
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
                int cost = 0;
                if (cy >= cost_lo && cy <= cost_hi && std::abs(cx - x) <= cost_reread.width / 2 &&
                    tr.text.size() <= 2 && utils::chars_to_number<int, true>(tr.text, cost) && cost >= 0) {
                    card.cost = cost;
                    continue;
                }
                if (cy >= empty_lo && cy <= empty_hi && tr.text.find("NO INFO") != std::string::npos) {
                    card.empty = true;
                    continue;
                }
                if (cy >= pact_lo && cy <= pact_hi && has_chinese(tr.text)) {
                    card.is_oper = true;
                    continue;
                }
                if (cy >= name_lo2 && cy <= name_hi2 && card.name.empty() && is_oper_name(tr.text)) {
                    card.name = tr.text;
                    LogTrace << "shop name matched" << tr.text << "card" << x;
                }
            }
        }
        if (card.cost < 0 && !card.empty) {
            // 费用数字常被漏检，用徽章区域重读
            RegionOCRer cost(image);
            cost.set_roi(card_rect(cost_reread, x));
            if (const auto result = cost.analyze()) {
                int value = 0;
                if (result->text.size() <= 2 && utils::chars_to_number<int, true>(result->text, value) && value >= 0) {
                    card.cost = value;
                }
            }
        }
        // 等阶未识别时按卡牌左上金框色判 V/VI
        if (card.tier == 0 &&
            color_ratio(image, card_rect(tier_frame, x), gold_lower_h, gold_upper_h, gold_lower_s, color_lower_v) >
                0.1) {
            card.tier = 5;
        }
        card.is_dps = is_dps_oper(card.name);
        card.is_oper = card.is_oper && !card.empty;
        cards.emplace_back(card);
        LogTrace << "shop card x" << x << "cost" << card.cost << "tier" << card.tier << "oper" << card.is_oper
                 << "empty" << card.empty << "name" << card.name;
    }
    return cards;
}

bool StrongholdProtocolTaskPlugin::buy_oper(const ShopCard& card)
{
    const auto gold0 = read_gold(ctrler()->get_image());
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
            break;
        }
    }
    if (saw_no_funds) {
        ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
        return false;
    }
    if (!saw_trait) {
        // 装备/非干员：取消选中后关提示
        ctrler()->click(Point(card.x, card_y));
        ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
        return false;
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
        gold1 = read_number(img, "StrongholdProtocol@ShopGoldOcr");
        if (gold0 && gold1 && *gold1 < *gold0) {
            break; // 降价即成交
        }
        if (card.cost == 0) {
            break; // 免费晋升赠送不走资金比对
        }
    }
    if (failed || (card.cost != 0 && gold0 && gold1 && *gold1 >= *gold0)) {
        ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
        return false;
    }
    if (card.cost == 0) {
        // 0 费（晋升赠送）没有资金变化可比：复读卡槽，同位置仍是未买走的 0 费干员说明没买成（多为费用误读成 0），
        // 不复核会被满级循环当作成功反复购买
        sleep(600);
        const auto cards = read_shop_cards(ctrler()->get_image());
        const bool still_there = std::ranges::any_of(cards, [&card](const ShopCard& c) {
            return std::abs(c.x - card.x) < 30 && !c.empty && c.is_oper && c.cost == 0;
        });
        if (still_there) {
            LogWarn << __FUNCTION__ << "free card still in shop, purchase not confirmed:" << card.name;
            ProcessTask(*this, { "StrongholdProtocol@PopupClose" }).run();
            return false;
        }
    }
    return true;
}

bool StrongholdProtocolTaskPlugin::buy_and_deploy(BoardState& board, const ShopCard& card)
{
    const auto before = ctrler()->get_image();
    if (!buy_oper(card)) {
        LogWarn << "buy failed:" << card.name << "cost" << card.cost;
        if (card.cost == 0) {
            board.pending_deploy = true; // 0 费复核判失败时也可能其实买到了，回合末扫一遍备战席兜底
        }
        return false;
    }
    LogInfo << "bought:" << card.name << "cost" << card.cost << "tier" << card.tier;
    sleep(600); // 等买入动画落到备战席再截图
    const auto after = ctrler()->get_image();

    // 当场没部署上的买入干员记下名字与等阶，回合末重扫部署时按名字恢复等阶
    const int tier_hint = card.tier >= 5 ? 5 : 1;
    const auto remember = [&board, &card, tier_hint] {
        if (!card.name.empty()) {
            board.bench_known.emplace_back(card.name, tier_hint);
        }
    };
    auto slot = new_bench_slot(before, after);
    if (!slot) {
        // 没找到新干员：可能直接合成晋升了，弹窗处理后待回合末扫描
        LogWarn << "new bench slot not found (maybe promoted)";
        handle_popups();
        remember();
        board.pending_deploy = true;
        return true;
    }
    if (!deploy_from_bench(board, *slot, card.name, tier_hint)) {
        remember();
        board.pending_deploy = true;
    }
    return true;
}

void StrongholdProtocolTaskPlugin::refresh_shop()
{
    ProcessTask(*this, { "StrongholdProtocol@RefreshClick" }).run();
    sleep(800); // 刷新有补货动画，等动画落定再让调用方读卡
}

// ---------------- 局内：回合准备主流程 ----------------

bool StrongholdProtocolTaskPlugin::prep_phase(BoardState& board)
{
    auto image = ctrler()->get_image();
    auto shop = read_shop(image);
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

    if (board.round_done < 0 && round == 1) {
        // 第 1 回合：买 2 个干员（最多 1 个医疗），其余的钱留给升级
        int bought = 0;
        for (const auto& card : read_shop_cards(image)) {
            if (bought >= 2) {
                break;
            }
            if (!card.is_oper || card.cost < 0 || (shop.gold && card.cost > *shop.gold)) {
                continue;
            }
            if (is_healer(card.name) && board.healers >= 1) {
                continue;
            }
            if (buy_and_deploy(board, card)) {
                ++bought;
                board.healers += is_healer(card.name);
            }
        }
        image = ctrler()->get_image();
        shop = read_shop(image);
    }

    if (!shop.maxed) {
        // 能升级就升级（同一轮可多次升级，最多连点 8 轮），成功判定：资金减少 / 费用变化 /
        // 按钮位移 / 满级。升级按钮走 OCR 任务（文字 ｢升级｣），商店满级后文字变 ｢满级｣ 即不再命中自动停
        for (int i = 0; i < 8 && !need_exit(); ++i) {
            image = ctrler()->get_image();
            shop = read_shop(image);
            if (shop.maxed || (shop.gold && shop.upgrade_cost && *shop.gold < *shop.upgrade_cost)) {
                break;
            }
            // 点 ｢升级｣ 后按钮变 ｢确认升级｣，需二次点击；text 子串命中，同一任务连点两次；
            // 扣费与按钮位移有动画，等落定再读，否则判定不中会误认为升级失败提前退出
            ProcessTask(*this, { "StrongholdProtocol@UpgradeClick" }).run();
            ProcessTask(*this, { "StrongholdProtocol@UpgradeClick" }).run();
            sleep(250);
            auto after = read_shop(ctrler()->get_image());
            auto upgraded_of = [&](const ShopInfo& a) {
                // 按钮随商店等级移动，位移超过 OCR 抖动即视为升级生效
                return a.maxed || (shop.upgrade_x && a.upgrade_x && std::abs(*a.upgrade_x - *shop.upgrade_x) > 10) ||
                       (shop.gold && a.gold && *a.gold < *shop.gold) ||
                       (shop.upgrade_cost && a.upgrade_cost && *a.upgrade_cost != *shop.upgrade_cost);
            };
            if (!upgraded_of(after)) {
                // 扣费与按钮位移有动画，首读未中再等一拍复读，仍不中才算失败
                sleep(250);
                after = read_shop(ctrler()->get_image());
            }
            const bool upgraded = upgraded_of(after);
            if (!upgraded) {
                break;
            }
            LogInfo << "shop upgraded, gold" << shop.gold.value_or(-1) << "->" << after.gold.value_or(-1);
        }
        image = ctrler()->get_image();
        shop = read_shop(image);
        if (!shop.maxed) {
            // 剩余资金买干员（资金不跨回合，不买装备）
            for (const auto& card : read_shop_cards(image)) {
                image = ctrler()->get_image();
                const auto gold = read_gold(image);
                const auto slots = slots_left(image);
                if (gold == 0 || slots == 0) {
                    break;
                }
                if (!card.is_oper || card.cost < 0 || (gold && card.cost > *gold)) {
                    continue;
                }
                if (is_healer(card.name) && board.healers >= 1) {
                    continue;
                }
                if (buy_and_deploy(board, card)) {
                    board.healers += is_healer(card.name);
                }
            }
        }
    }

    if (shop.maxed) {
        // 满级后只买 V/VI 干员，没有就刷新；优先免费（晋升赠送）> 权重 > 凑晋升 > 输出型
        std::string last_signature;
        int same_count = 0;
        for (int i = 0; i < 40 && !need_exit(); ++i) {
            image = ctrler()->get_image();
            const auto gold = read_gold(image);
            const auto cards = read_shop_cards(image);
            std::string signature;
            for (const auto& card : cards) {
                signature += card.name + ":" + std::to_string(card.cost) + ",";
            }
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

            const int gold_value = gold.value_or(99);
            const ShopCard* pick = nullptr;
            // 优先免费干员
            for (const auto& card : cards) {
                if (card.is_oper && card.cost == 0) {
                    if (!pick || oper_rank(card.name, card.tier) > oper_rank(pick->name, pick->tier)) {
                        pick = &card;
                    }
                }
            }
            if (!pick) {
                // 权重优先 > 已拥有（凑 3 晋升）> 输出型 > 买得起 4 费时的非输出
                auto affordable = [&](const ShopCard& c) {
                    return c.is_oper && c.tier >= 5 && c.cost >= 0 && c.cost <= gold_value;
                };
                int best_priority = INT_MAX;
                for (const auto& card : cards) {
                    if (!affordable(card)) {
                        continue;
                    }
                    int p = oper_priority(card.name);
                    if (p >= 0 && p < best_priority) {
                        pick = &card;
                        best_priority = p;
                    }
                }
                if (!pick) {
                    for (const auto& card : cards) {
                        if (!affordable(card) || card.cost == 0) {
                            continue;
                        }
                        const bool owned = !card.name.empty() && std::ranges::any_of(board.owned, [&](const auto& kv) {
                            return !kv.first.empty() && kv.first == card.name;
                        });
                        if (owned) {
                            pick = &card;
                            break;
                        }
                    }
                }
                if (!pick) {
                    for (const auto& card : cards) {
                        if (affordable(card) && card.cost > 0 && card.is_dps) {
                            pick = &card;
                            break;
                        }
                    }
                }
                // 资金见底（扣 1 刷新费后不足 4）才买任意可买卡清仓；资金充足时留钱刷新等 V/VI
                if (!pick && gold && *gold - 1 < 4) {
                    for (const auto& card : cards) {
                        if (affordable(card) && card.cost > 0) {
                            pick = &card;
                            break;
                        }
                    }
                }
            }
            if (pick) {
                LogInfo << "buy V/VI:" << pick->name << "cost" << pick->cost;
                if (buy_and_deploy(board, *pick)) {
                    ++board.owned[pick->name];
                    last_signature.clear(); // 商店已变化，重置签名
                    continue;
                }
                LogWarn << "buy failed, stop purchasing this round";
                break;
            }
            if (gold && *gold < 4) {
                break;
            }
            refresh_shop();
        }
    }

    if (board.pending_deploy) {
        deploy_bench(board);
        board.pending_deploy = false;
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
    emit_input(InputEvent::Type::WAIT_MS, to, hold_ms / 2); // 终点按住让游戏确认落格
    emit_input(InputEvent::Type::TOUCH_UP, to);
}

std::optional<Point> StrongholdProtocolTaskPlugin::deploy_from_bench(
    BoardState& board,
    const Point& bench,
    const std::string& name_hint,
    int tier_hint)
{
    std::string name = name_hint; // 拖拽面板读到的名字更可信，复核时覆盖
    int tier = tier_hint;
    if (oper_priority(name) >= 0) {
        tier = std::max(tier, 5); // 优先干员都是 V/VI，不当作低星卖掉
    }
    // 放置前基线：意外交换检测（slots 差分）与换下者定位（板凳差分）都用它
    const auto before_deploy = ctrler()->get_image();
    const auto slots0 = slots_left(before_deploy);

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
    const auto& tiles = StrongholdProtocol.get_tiles();
    const Point lp = tiles.lift;
    const int lift_ms = std::max(150, static_cast<int>(Point::distance(probe, lp)) / 2);
    for (int i = 1; i <= 8; ++i) {
        emit_input(
            InputEvent::Type::TOUCH_MOVE,
            Point { probe.x + (lp.x - probe.x) * i / 8, probe.y + (lp.y - probe.y) * i / 8 });
        emit_input(InputEvent::Type::WAIT_MS, lp, std::max(16, lift_ms / 8));
    }
    // 拖拽预览与面板在抓取激活时就开始渲染，拖动过程中已完成，到悬停点直接截图复核
    const auto preview = ctrler()->get_image();

    // 拿起复核：左侧面板出现特质/技能等标签、读到干员名，或可部署格高亮，才认为抓到了东西
    std::vector<std::string> trait_tabs;
    {
        OCRer tabs(preview);
        tabs.set_task_info("StrongholdProtocol@DragTraitOcr");
        if (auto results = tabs.analyze()) {
            for (const auto& tr : *results) {
                trait_tabs.emplace_back(tr.text);
            }
        }
    }
    const auto read_drag_name = [](const cv::Mat& img) -> std::string {
        // 面板名字行里找干员名：含中文/字母且不是 LV 等级行
        OCRer ocr(img);
        ocr.set_task_info("StrongholdProtocol@DragNameOcr");
        const auto results = ocr.analyze();
        if (!results) {
            return {};
        }
        for (const auto& tr : *results) {
            const std::string& t = tr.text;
            const bool has_word =
                std::ranges::any_of(t, [](unsigned char c) { return c >= 0x80; }) ||
                std::ranges::any_of(t, [](char c) { return std::isalpha(static_cast<unsigned char>(c)); });
            if (has_word && t.find("LV") == std::string::npos && t.find("lv") == std::string::npos) {
                return t;
            }
        }
        return {};
    };
    const std::string drag_name = read_drag_name(preview);
    if (!drag_name.empty()) {
        name = drag_name;
        if (oper_priority(name) >= 0) {
            tier = std::max(tier, 5);
        }
    }
    // 格表里任一格变绿即进入了拖拽预览
    const auto any_tile_green = [&]() {
        for (const auto* group : { &tiles.safe, &tiles.path, &tiles.rest }) {
            if (std::ranges::any_of(*group, [&](const Point& p) { return is_green_tile(preview, p); })) {
                return true;
            }
        }
        return false;
    };
    // 判定只看本次预览读到的东西，不看 name_hint（否则带提示名的购买路径永远判不出没拿起）
    if (trait_tabs.empty() && drag_name.empty() && !any_tile_green()) {
        // 没抓到任何东西（按压点无干员）：抬手放弃，存 preview 供排查
        emit_input(InputEvent::Type::TOUCH_UP, lp);
        save_img(utils::path("debug") / utils::path("stronghold")); // 截图缓存即 preview
        LogWarn << "deploy preview not entered (grab missed):" << name;
        return std::nullopt;
    }
    // 装备判定：面板没有 ｢特质/天赋｣ 标签、或名字以引号开头（召唤物/装备名）的当装备放回
    const bool has_oper_tab = std::ranges::any_of(trait_tabs, [](const std::string& t) {
        return t.find("特质") != std::string::npos || t.find("天赋") != std::string::npos;
    });
    const bool quoted_name =
        name.starts_with("“") || name.starts_with("「") || name.starts_with("\"") || name.starts_with("『");
    if (!has_oper_tab || quoted_name) {
        emit_input(InputEvent::Type::TOUCH_UP, lp);
        sleep(800);
        LogInfo << "picked up equipment, returned:" << name;
        return std::nullopt;
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
    // 回合末重扫部署时按名字找回买入或换下时记录的等阶（面板黄框漏判时兜底）
    const auto same_oper = [](const std::string& a, const std::string& b) {
        return !a.empty() && a == b;
    };
    std::string known_name;
    if (const auto it =
            std::ranges::find_if(board.bench_known, [&](const auto& k) { return same_oper(k.first, name); });
        it != board.bench_known.end()) {
        tier = std::max(tier, it->second);
        known_name = it->first;
    }
    // 已确认是干员：等阶最低按 I 记，不记 0
    tier = std::max(tier, 1);

    // 固定格表选格（tiles 数据文件经 StrongholdProtocol.get_tiles() 读入）：
    // path=最右 3 列最上两行敌人路径（主堵上路），rest=右侧其余（下路补充），safe=敌人不经过的高台。
    // 安全干员优先 safe 组，其余优先 path+rest 组；组内先找空格，无空格再换本组已记录的低星。
    // 空位判定：occ 已记录即占；未记录的格以预览中绿高亮采样判定（绿=空，无绿=有人）。
    // 未记录的占用一律不换，防误卖场上高星。
    const auto tile_green = [&preview](const Point& c) {
        return is_green_tile(preview, c);
    };
    const bool safe_oper = is_safe_oper(name);
    // 换人候选按价值排序：只换下已记录的低星（I~IV，未知占位不换），挑价值最低且低于当前干员的；
    // 安全干员遇到被非安全干员占着的安全位时，该格价值减 2000 优先换下
    const int my_rank = oper_rank(name, tier);
    const auto occ_value = [&](const std::string& key, const std::pair<std::string, int>& occ) {
        int value = oper_rank(occ.first, occ.second);
        if (key.starts_with("safe") && safe_oper && !is_safe_oper(occ.first)) {
            value -= 2000;
        }
        return value;
    };
    // 格组（组名, 格列表），occ 键为 ｢组名+序号｣
    const std::vector<std::pair<std::string, std::vector<Point>>> front_groups = { { "path", tiles.path },
                                                                                   { "rest", tiles.rest } };
    const std::vector<std::pair<std::string, std::vector<Point>>> safe_groups = { { "safe", tiles.safe } };
    const auto& primary = safe_oper ? safe_groups : front_groups;
    const auto& secondary = safe_oper ? front_groups : safe_groups;
    std::string target_key;
    Point target_pt(-1, -1);
    std::pair<std::string, int> swap_old; // 非空 name 表示本次放置是换下 occ 记录的旧干员
    const auto pick_in = [&](const std::vector<std::pair<std::string, std::vector<Point>>>& groups) {
        for (const auto& [group_name, points] : groups) {
            for (size_t i = 0; i < points.size(); ++i) {
                // 医疗不放堵口位（path[0]）：零输出会把怪卡住直到回合倒计时耗尽
                if (group_name == "path" && i == 0 && is_healer(name)) {
                    continue;
                }
                const std::string key = group_name + std::to_string(i);
                if (!board.occ.contains(key) && tile_green(points[i])) {
                    target_key = key;
                    target_pt = points[i];
                    return;
                }
            }
            std::optional<std::pair<int, size_t>> best; // (价值, 组内序号)，同价值取靠前的格
            for (size_t i = 0; i < points.size(); ++i) {
                const auto it = board.occ.find(group_name + std::to_string(i));
                if (it == board.occ.end() || it->second.second >= 5 || it->second.first == "?") {
                    continue;
                }
                const int value = occ_value(it->first, it->second);
                if (value < my_rank && (!best || value < best->first)) {
                    best = std::pair { value, i };
                }
            }
            if (best) {
                target_key = group_name + std::to_string(best->second);
                target_pt = points[best->second];
                swap_old = board.occ.at(target_key);
                return;
            }
        }
    };
    pick_in(primary);
    if (target_key.empty()) {
        pick_in(secondary);
    }
    if (target_key.empty()) {
        // 无处可放：抬手放回备战席（悬停点在不可部署区域，松手即取消放置）
        emit_input(InputEvent::Type::WAIT_MS, lp, 300);
        emit_input(InputEvent::Type::TOUCH_UP, lp);
        LogInfo << "no free tile, returned to bench:" << name;
        return std::nullopt;
    }
    LogInfo << "deploy target" << target_key << target_pt.x << "," << target_pt.y << "safe" << safe_oper;

    // 已按住预览（当前触点 lp）：滑到目标格后松手（move 触发游戏格内吸附）
    const int dist = static_cast<int>(Point::distance(lp, target_pt));
    const int move_ms = std::max(300, dist);
    const int steps = 8;
    const int per = std::max(16, move_ms / steps);
    for (int i = 1; i <= steps; ++i) {
        emit_input(
            InputEvent::Type::TOUCH_MOVE,
            Point { lp.x + (target_pt.x - lp.x) * i / steps, lp.y + (target_pt.y - lp.y) * i / steps });
        emit_input(InputEvent::Type::WAIT_MS, target_pt, per);
    }
    emit_input(InputEvent::Type::WAIT_MS, target_pt, 150); // 终点按住让触点停稳再松手
    emit_input(InputEvent::Type::TOUCH_UP, target_pt);

    // 放置成功会停在 ｢选择朝向｣ 界面（出现 ｢点击取消｣），轮询 ≤2s
    bool facing_shown = false;
    for (int i = 0; i < 10 && !need_exit(); ++i) {
        sleep(200);
        OCRer facing(ctrler()->get_image());
        facing.set_task_info("StrongholdProtocol@FacingCancelOcr");
        if (auto r = facing.analyze(); r && !r->empty()) {
            facing_shown = true;
            break;
        }
    }
    if (!facing_shown) {
        save_img(utils::path("debug") / utils::path("stronghold"), false);
        LogWarn << "deploy failed (probably equipment or occupied tile):" << name;
        return std::nullopt;
    }

    set_facing(target_pt);
    m_last_tile = target_pt;

    // 格上现在是刚放下的干员（换人与意外交换都一样），被顶下者回到备战席
    const auto after_deploy = ctrler()->get_image();
    const auto slots1 = slots_left(after_deploy);
    const bool is_swap = !swap_old.first.empty();
    board.occ[target_key] = { name, tier };
    if (!known_name.empty()) {
        if (const auto it = std::ranges::find(board.bench_known, known_name, &std::pair<std::string, int>::first);
            it != board.bench_known.end()) {
            board.bench_known.erase(it);
        }
    }
    // 意外交换检测：非换人放置但空位数没减少，说明目标格原本有未记录的干员，身份未知不卖，回合末重扫部署
    if (!is_swap && slots0 && slots1 && *slots1 >= *slots0) {
        board.pending_deploy = true;
        LogWarn << "unexpected swap at" << target_key << ", displaced occupant left for round-end scan";
        return target_pt;
    }

    if (is_swap) {
        // 顶下来的旧干员回到备战席：先标记回合末重扫（定位或卖出失败时兜底），低星当场拖去出售区（资金增加确认卖出）
        board.pending_deploy = true;
        bool sold = false;
        if (swap_old.second < 5) {
            if (auto out = new_bench_slot(before_deploy, after_deploy)) {
                const auto gold0 = read_number(ctrler()->get_image(), "StrongholdProtocol@ShopGoldOcr");
                const Rect sell = task_roi("StrongholdProtocol@Roi@ShopSell");
                drag_to(*out, Point(sell.x + sell.width / 2, sell.y + sell.height / 2), 400, 250);
                sleep(1000);
                const auto gold1 = read_number(ctrler()->get_image(), "StrongholdProtocol@ShopGoldOcr");
                sold = gold0 && gold1 && *gold1 > *gold0;
                LogInfo << "sold swapped-out low-tier oper" << swap_old.first << (sold ? "ok" : "unconfirmed");
            }
            else {
                LogWarn << "swapped-out oper not located on bench, left for round-end scan:" << swap_old.first;
            }
        }
        // 没卖掉的被顶下者记下身份与等阶，回合末重扫部署时恢复（高星不会因面板漏判被当成低星换掉）
        if (!sold) {
            board.bench_known.emplace_back(swap_old);
        }
    }
    LogInfo << "deployed" << (name.empty() ? "?" : name) << "tier" << tier << "to" << target_key << target_pt.x << ","
            << target_pt.y;
    return target_pt;
}

void StrongholdProtocolTaskPlugin::deploy_bench(BoardState& board)
{
    // 备战席小人按头顶等阶方牌检测定位（台面布局随人数变化，固定扫点不可靠）：
    // 每轮重新检测当前剩余人，逐个尝试部署，部署成功小人消失故循环重扫
    for (int round = 0; round < 10 && !need_exit(); ++round) {
        const auto opers = detect_bench_opers(ctrler()->get_image());
        if (opers.empty()) {
            break;
        }
        bool placed = false;
        for (const auto& pt : opers) {
            if (deploy_from_bench(board, pt)) {
                placed = true;
                break;
            }
        }
        if (!placed) {
            break;
        }
    }
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
std::vector<Point> StrongholdProtocolTaskPlugin::detect_bench_opers(const cv::Mat& image)
{
    const Rect scan = task_roi("StrongholdProtocol@Roi@BenchScan");
    const Rect press = task_roi("StrongholdProtocol@Roi@BenchPressOffset");
    const cv::Rect scan_cv = make_rect<cv::Rect>(scan) & cv::Rect(0, 0, image.cols, image.rows);
    const cv::Mat sub = image(scan_cv);

    // 等阶方牌是深灰圆角方块（面 ~30，远低于木台面 ~70）：低亮度粗筛候选块
    cv::Mat gray;
    cv::cvtColor(sub, gray, cv::COLOR_BGR2GRAY);
    cv::Mat dark;
    cv::threshold(gray, dark, 60, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(dark, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<Point> opers;
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
        // 牌面整体须是深色（排除亮块边框），内部须有浅色罗马数字笔画（~150-190 非纯白）
        const cv::Rect inner(rc.x + 4, rc.y + 4, rc.width - 8, rc.height - 8);
        if (inner.width <= 0 || inner.height <= 0) {
            continue;
        }
        const cv::Mat inner_gray = gray(inner);
        if (cv::mean(inner_gray)[0] > 90) {
            continue;
        }
        cv::Mat white;
        cv::threshold(inner_gray, white, 140, 255, cv::THRESH_BINARY);
        if (cv::countNonZero(white) < inner.area() * 0.03) {
            continue;
        }
        // 按压点：方牌左上角加 Roi@BenchPressOffset 偏移落到小人躯干（加回扫描区偏移）
        const Point pt(scan_cv.x + rc.x + press.x, scan_cv.y + rc.y + press.y);
        LogTrace << "bench tile" << (scan_cv.x + rc.x) << (scan_cv.y + rc.y) << rc.width << rc.height << "press" << pt.x
                 << pt.y;
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
    const bool count_this = !m_settlement_counted;
    if (count_this) {
        m_settlement_counted = true;
        // ｢模拟完成｣ 出现在结算页则为通关
        OCRer result(ctrler()->get_image());
        result.set_task_info("StrongholdProtocol@SettlementResultOcr");
        const bool cleared = result.analyze().has_value();
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
    return count_this;
}
} // namespace asst
