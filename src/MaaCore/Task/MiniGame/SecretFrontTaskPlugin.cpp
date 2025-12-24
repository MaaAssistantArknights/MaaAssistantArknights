#include "SecretFrontTaskPlugin.h"

#include <algorithm>
#include <cmath>
#include <string_view>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/RegionOCRer.h"

namespace asst
{
// 阶段目标
constexpr std::array<int, 3> Target_1A { 20, 20, 20 };
constexpr std::array<int, 3> Target_2A { 60, 55, 45 };
constexpr std::array<int, 3> Target_2B { 45, 55, 65 };
constexpr std::array<int, 3> Target_3A { 200, 155, 155 };
constexpr std::array<int, 3> Target_3B { 150, 220, 250 };
constexpr std::array<int, 3> Target_3C { 155, 255, 240 };
// 结局目标
constexpr std::array<int, 3> Target_EndA { 510, 350, 350 };
constexpr std::array<int, 3> Target_EndB { 440, 440, 350 };
constexpr std::array<int, 3> Target_EndC { 350, 550, 350 };
constexpr std::array<int, 3> Target_EndD { 365, 475, 495 };
constexpr std::array<int, 3> Target_EndE { 370, 385, 620 };

constexpr std::string_view HookTaskLegacy = "MiniGame@SecretFront@Begin";
constexpr std::string_view HookTaskA = "MiniGame@SecretFront@EndingA@Begin";
constexpr std::string_view HookTaskB = "MiniGame@SecretFront@EndingB@Begin";
constexpr std::string_view HookTaskC = "MiniGame@SecretFront@EndingC@Begin";
constexpr std::string_view HookTaskD = "MiniGame@SecretFront@EndingD@Begin";
constexpr std::string_view HookTaskE = "MiniGame@SecretFront@EndingE@Begin";

constexpr std::string_view RouteA[] { "1A", "2A", "3A", "4A" };
constexpr std::string_view RouteB[] { "1A", "2A", "3A", "4B" };
constexpr std::string_view RouteC[] { "1A", "2A", "3B", "4C" };
constexpr std::string_view RouteD[] { "1A", "2B", "3C", "4D" };
constexpr std::string_view RouteE[] { "1A", "2B", "3C", "4E" };

constexpr SecretFrontTaskPlugin::EndingInfo EndingTable[] {
    { HookTaskA, Target_EndA, SecretFrontTaskPlugin::Ending::A },
    { HookTaskB, Target_EndB, SecretFrontTaskPlugin::Ending::B },
    { HookTaskC, Target_EndC, SecretFrontTaskPlugin::Ending::C },
    { HookTaskD, Target_EndD, SecretFrontTaskPlugin::Ending::D },
    { HookTaskE, Target_EndE, SecretFrontTaskPlugin::Ending::E },
    { HookTaskLegacy, Target_EndE, SecretFrontTaskPlugin::Ending::E },
};

constexpr std::pair<std::string_view, std::array<int, 3>> StageTargets[] {
    { "1A", Target_1A }, { "2A", Target_2A }, { "2B", Target_2B },
    { "3A", Target_3A }, { "3B", Target_3B }, { "3C", Target_3C },
};

bool SecretFrontTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    // 只处理 ProcessTask
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");

    // 1) 若命中结局入口（Begin 节点），仅用于确定 m_ending / m_target，不触发插件运行
    for (const auto& e : EndingTable) {
        if (!task.ends_with(e.hook)) {
            continue;
        }

        m_target = e.target;
        m_ending = e.ending;

        return false;
    }

    // 2) 卡片判断：区分有数值的 Actions 与基于文本的 Event
    if (task.ends_with("MiniGame@SecretFront@ActionsDetected")) {
        m_mode = Mode::Actions;
        Log.info(__FUNCTION__, "| detect Actions");
        return true;
    }
    if (task.ends_with("MiniGame@SecretFront@EventDetected")) {
        m_mode = Mode::Event;
        Log.info(__FUNCTION__, "| detect Event");
        return true;
    }

    // 3) SelectTeam 进入插件，由插件根据结局选择对应分队
    if (task.ends_with("MiniGame@SecretFront@SelectTeam")) {
        m_mode = Mode::SelectTeam;
        return true;
    }

    return false;
}

Point SecretFrontTaskPlugin::card_pos_base(int total, int idx)
{
    if (total == 3) {
        static constexpr Point pos[] { { 200, 311 }, { 553, 311 }, { 907, 311 } };
        return pos[std::clamp(idx, 0, 2)];
    }
    if (total == 2) {
        static constexpr Point pos[] { { 377, 311 }, { 730, 311 } };
        return pos[std::clamp(idx, 0, 1)];
    }
    return { 553, 311 };
}

std::optional<int> SecretFrontTaskPlugin::ocr_int(const cv::Mat& image, const Rect& roi) const
{
    RegionOCRer ocr(image);
    ocr.set_task_info("MiniGame@SecretFront@Number");
    ocr.set_roi(roi);
    if (!ocr.analyze()) {
        return std::nullopt;
    }

    int value = 0;
    if (!utils::chars_to_number(ocr.get_result().text, value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::array<int, 3>> SecretFrontTaskPlugin::read_properties(const cv::Mat& image) const
{
    const std::array<TaskPtr, 3> tasks { Task.get("MiniGame@SecretFront@ReadProperty-Materiel"),
                                         Task.get("MiniGame@SecretFront@ReadProperty-Intelligence"),
                                         Task.get("MiniGame@SecretFront@ReadProperty-Medicine") };
    std::array<int, 3> v {};
    for (int i = 0; i < 3; ++i) {
        auto r = ocr_int(image, tasks[i]->roi);
        if (!r) {
            return std::nullopt;
        }
        v[i] = *r;
    }

    Log.info(__FUNCTION__, "| properties materiel=", v[0], " intelligence=", v[1], " medicine=", v[2]);

    return v;
}

std::optional<SecretFrontTaskPlugin::CardData>
    SecretFrontTaskPlugin::read_card(const cv::Mat& image, int total, int idx) const
{
    const auto base = card_pos_base(total, idx);
    const Rect base_rect { base.x, base.y, 0, 0 };

    const auto& materiel_r = Task.get("MiniGame@SecretFront@ReadCard-Materiel")->rect_move;
    const auto& intelligence_r = Task.get("MiniGame@SecretFront@ReadCard-Intelligence")->rect_move;
    const auto& medicine_r = Task.get("MiniGame@SecretFront@ReadCard-Medicine")->rect_move;
    const auto& percent_r = Task.get("MiniGame@SecretFront@ReadCard-Percentage")->rect_move;

    auto materiel = ocr_int(image, base_rect.move(materiel_r));
    auto intelligence = ocr_int(image, base_rect.move(intelligence_r));
    auto medicine = ocr_int(image, base_rect.move(medicine_r));
    auto percent_int = ocr_int(image, base_rect.move(percent_r));

    // 允许百分比识别失败：按 100% 兜底
    double p = 1.0;
    if (percent_int) {
        p = std::clamp(static_cast<double>(*percent_int) / 100.0, 0.0, 1.0);
    }

    if (!materiel || !intelligence || !medicine) {
        return std::nullopt;
    }

    LogInfo << __FUNCTION__ << "| card" << idx << "materiel=" << *materiel << "intelligence=" << *intelligence
            << "medicine=" << *medicine << "percentage=" << p;

    return CardData { .materiel = *materiel, .intelligence = *intelligence, .medicine = *medicine, .percentage = p };
}

int SecretFrontTaskPlugin::estimate_total_cards(const cv::Mat& image) const
{
    // 尝试从最多卡片数开始检测
    for (int total = 3; total >= 1; --total) {
        int ok = 0;
        for (int i = 0; i < total; ++i) {
            if (read_card(image, total, i)) {
                ++ok;
            }
        }

        if (ok == total) { // 所有卡片都能读取
            Log.info(__FUNCTION__, "| estimate_total_cards -> ", total, " (all ", total, " cards read)");
            return total;
        }
    }

    // 如果3、2都不行，默认返回1
    Log.info(__FUNCTION__, "| estimate_total_cards -> 1 (default)");
    return 1;
}

std::array<int, 3> SecretFrontTaskPlugin::get_current_stage_target(const std::array<int, 3>& properties) const
{
    const auto route = current_route();

    auto find_stage_target = [](std::string_view stage) -> const std::array<int, 3>* {
        for (const auto& [name, target] : StageTargets) {
            if (stage == name) {
                return &target;
            }
        }
        return nullptr;
    };

    for (auto stage : route) {
        if (auto t = find_stage_target(stage)) {
            const bool reached =
                std::equal(properties.begin(), properties.end(), t->begin(), [](int p, int s) { return p >= s; });

            if (!reached) {
                return *t;
            }
        }
    }

    return m_target;
}

SecretFrontTaskPlugin::BestChoice SecretFrontTaskPlugin::choose_best_card(
    const std::array<int, 3>& properties,
    int current_total,
    const std::vector<CardData>& cards)
{
    // 计算当前阶段目标与各项距离的权重
    const auto current_target = get_current_stage_target(properties);

    auto calc_score = [&](const CardData& card) {
        std::array<double, 3> d {};
        for (size_t i = 0; i < 3; ++i) {
            d[i] = std::max(0.0, static_cast<double>(current_target[i] - properties[i]));
        }
        const double denom = d[0] + d[1] + d[2];
        if (denom <= 0.0) {
            return 0.0;
        }
        const auto gain = card.expected_gain();
        double score = 0.0;
        for (size_t k = 0; k < 3; ++k) {
            score += gain[k] * (d[k] / denom);
        }
        return score;
    };

    BestChoice best;
    best.page = 0;
    best.total = current_total;
    best.idx = 0;
    best.score = -1.0;

    int last_page = 0;

    // 当前页评分
    for (int idx = 0; idx < current_total; ++idx) {
        double sc = calc_score(cards[idx]);
        if (sc > best.score) {
            best.score = sc;
            best.page = 0;
            best.total = current_total;
            best.idx = idx;
        }
    }

    // 尝试遍历后续两页进行全局比较（翻页操作）
    for (int page = 1; page < 3; ++page) {
        ProcessTask(*this, { "MiniGame@SecretFront@ClickNextPage" }).run();
        sleep(1000);

        auto image = ctrler()->get_image();
        if (image.empty()) {
            break;
        }

        const int t = estimate_total_cards(image);
        if (t <= 0) {
            continue;
        }

        last_page = page;

        std::vector<CardData> page_cards;
        page_cards.reserve(t);
        for (int i = 0; i < t; ++i) {
            if (auto card = read_card(image, t, i)) {
                page_cards.emplace_back(*card);
            }
            else {
                page_cards.emplace_back(CardData {});
            }
        }

        for (int idx = 0; idx < t; ++idx) {
            double sc = calc_score(page_cards[idx]);
            if (sc > best.score) {
                best.score = sc;
                best.page = page;
                best.total = t;
                best.idx = idx;
            }
        }
    }

    if (last_page != best.page) {
        if (best.page == 0) {
            ProcessTask(*this, { "MiniGame@SecretFront@ClickNextPage" }).run();
        }
        else if (best.page == 1) {
            ProcessTask(*this, { "MiniGame@SecretFront@ClickPrevPage" }).run();
        }

        sleep(1000);
    }

    return best;
}

void SecretFrontTaskPlugin::click_choose_card(int total, int idx) const
{
    int start = 700;
    const int step = 350;
    if (total == 3) {
        start = 350;
    }
    else if (total == 2) {
        start = 520;
    }

    const Point p { start + idx * step, 600 };
    Log.info(__FUNCTION__, "| click_choose_card total=", total, ", idx=", idx);
    for (int i = 0; i < 2; ++i) {
        ctrler()->click(p);
        sleep(500);
    }
}

int SecretFrontTaskPlugin::estimate_total_cards_event(const cv::Mat& image) const
{
    for (int t = 3; t >= 2; --t) {
        if (!read_stage_name(image, t, t - 1, 100).empty()) {
            Log.info(__FUNCTION__, "| estimate_total_cards_event -> ", t);
            return t;
        }
    }
    Log.info(__FUNCTION__, "| estimate_total_cards_event -> 1");
    return 1;
}

std::string SecretFrontTaskPlugin::read_stage_name(const cv::Mat& image, int total, int idx, int bin_threshold) const
{
    // 读取卡牌左上角的一小块标题区域，通过文字 OCR 获取 "1A/1B/2A" 等
    const auto base = card_pos_base(total, idx);
    auto title_r = Task.get("MiniGame@SecretFront@EventReplace")->roi;
    title_r.x += base.x;
    title_r.y += base.y;

    RegionOCRer ocr(image);
    ocr.set_roi(title_r);
    if (bin_threshold >= 0) {
        ocr.set_bin_threshold(bin_threshold, 255);
    }
    ocr.set_replace(Task.get<OcrTaskInfo>("MiniGame@SecretFront@EventReplace")->replace_map);
    if (!ocr.analyze()) {
        return {};
    }
    std::string text = ocr.get_result().text;

    // 只保留类似 "1A/1B/2A/2B/3A/3B/3C" 的短标识
    std::string short_name;
    for (char ch : text) {
        if ((ch >= '1' && ch <= '3') || (ch >= 'A' && ch <= 'E') || (ch >= 'a' && ch <= 'e')) {
            short_name.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }

    // 尝试提取形如 '1A' 的前两个字符
    if (short_name.size() >= 2) {
        return short_name.substr(0, 2);
    }
    return {};
}

std::vector<std::string_view> SecretFrontTaskPlugin::current_route() const
{
    switch (m_ending) {
    case Ending::A:
        return { std::begin(RouteA), std::end(RouteA) };
    case Ending::B:
        return { std::begin(RouteB), std::end(RouteB) };
    case Ending::C:
        return { std::begin(RouteC), std::end(RouteC) };
    case Ending::D:
        return { std::begin(RouteD), std::end(RouteD) };
    case Ending::E:
    default:
        return { std::begin(RouteE), std::end(RouteE) };
    }
}

bool SecretFrontTaskPlugin::handle_event_page(const cv::Mat& image) const
{
    const int total = estimate_total_cards_event(image);
    if (total <= 0) {
        return false;
    }

    const auto route = current_route();
    if (route.empty()) {
        return false;
    }

    // 读取每一张卡的阶段名（1A/1B/2A...），选择在路线中的那一张
    for (int idx = 0; idx < total; ++idx) {
        const auto name = read_stage_name(image, total, idx);
        if (name.empty()) {
            continue;
        }

        const bool in_route = std::any_of(route.begin(), route.end(), [&](std::string_view r) {
            return r.find(name) != std::string_view::npos;
        });
        if (in_route) {
            Log.info(__FUNCTION__, "| event page choose idx=", idx, ", name=", name);
            click_choose_card(total, idx);
            return true;
        }
    }

    // 如果都不在路线内，保底选第一张
    Log.info(__FUNCTION__, "| event page no route match, fallback idx=0");
    click_choose_card(total, 0);
    return true;
}

bool SecretFrontTaskPlugin::_run()
{
    LogTraceFunction;

    // SelectTeam 场景：根据当前结局选择对应分队（物资/情报/医疗）
    if (m_mode == Mode::SelectTeam) {
        std::string team_task;
        switch (m_ending) {
        case Ending::A:
        case Ending::B:
            team_task = "MiniGame@SecretFront@TapTeam@Management";
            break;
        case Ending::C:
            team_task = "MiniGame@SecretFront@TapTeam@Intelligence";
            break;
        case Ending::D:
        case Ending::E:
        default:
            team_task = "MiniGame@SecretFront@TapTeam@Medicine";
            break;
        }

        Log.info(__FUNCTION__, "| select team by ending, task=", team_task);
        // 由插件主动执行分队选择与确认，避免 JSON 中循环 next 再进插件
        bool ok = ProcessTask(*this, { team_task, "MiniGame@SecretFront@SelectTeamOK" }).run();
        return ok;
    }

    auto image = ctrler()->get_image();
    if (image.empty()) {
        return false;
    }

    // 如果是 Event 模式，直接按事件页文本处理
    if (m_mode == Mode::Event) {
        Log.info(__FUNCTION__, "| event mode: handle by text OCR");
        return handle_event_page(image);
    }

    // Actions 模式：按数值处理
    auto properties = read_properties(image);
    if (!properties) {
        Log.trace(__FUNCTION__, "| unable to read properties, skip");
        return true;
    }

    int total = estimate_total_cards(image);
    std::vector<CardData> cards;
    cards.reserve(std::max(total, 0));

    int non_zero_cards = 0;
    for (int i = 0; i < total; ++i) {
        if (auto card = read_card(image, total, i)) {
            cards.emplace_back(*card);
            if (card->materiel > 0 || card->intelligence > 0 || card->medicine > 0) {
                ++non_zero_cards;
            }
        }
        else {
            cards.emplace_back(CardData {});
        }
    }

    if (non_zero_cards == 0) {
        // Actions 模式下未读取到任何数值，直接跳过
        Log.info(__FUNCTION__, "| no numeric values found in Actions mode, skip");
        return true;
    }

    // 从当前页与后续两页中选择全局最优卡片（逻辑已移动到 choose_best_card）
    auto best = choose_best_card(*properties, total, cards);
    Log.info(__FUNCTION__, "| best_page=", best.page, ", total=", best.total, ", best_idx=", best.idx);

    sleep(500);
    click_choose_card(best.total, std::clamp(best.idx, 0, best.total - 1));

    return true;
}
}
