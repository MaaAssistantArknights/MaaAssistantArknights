#include "InfrastPresetTask.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ranges>
#include <tuple>
#include <unordered_set>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"

namespace
{
using asst::Point;
using asst::Rect;

// 设施名称列，尽量避开右侧产物/状态文字
const Rect WorkAreaRoomTextRoi = { 400, 55, 150, 650 };
const Rect SwitchButtonSearchRoi = { 1160, 60, 70, 640 };
constexpr int MaxScrollTimes = 16;
constexpr int BottomBlockedY = 610;
constexpr int SwitchButtonMaxYDrift = 70;
constexpr int SwitchButtonClickOffsetX = 3;
constexpr int SwitchButtonMinHeight = 28;
// 列表里设施名 OCR 在卡片顶部，切换按钮在整行垂直居中，需按整行高度估算按钮 Y。
constexpr int FacilityRowHeight = 88;
// 模板匹配成功时，设施名 rect 顶边到切换按钮顶边的实测偏移（1280x720）。
constexpr int SwitchButtonTopOffsetY = 27;
constexpr int SwitchButtonWidth = 42;
constexpr int SwitchButtonHeight = 46;
constexpr int SwitchButtonLeftX = 1188;
constexpr double SwitchButtonGeometryVerifyThreshold = 0.65;

int switch_button_expected_y(const Rect& room_text_rect)
{
    return room_text_rect.y + SwitchButtonTopOffsetY + SwitchButtonHeight / 2;
}

std::pair<int, int> switch_button_search_y_range(const Rect& room_text_rect)
{
    const int y_begin = std::max(SwitchButtonSearchRoi.y, room_text_rect.y - 10);
    const int y_end = std::min(
        SwitchButtonSearchRoi.y + SwitchButtonSearchRoi.height,
        room_text_rect.y + FacilityRowHeight + 20);
    return { y_begin, y_end };
}

bool is_room_row_blocked_by_bottom(const Rect& room_text_rect)
{
    return room_text_rect.y + FacilityRowHeight > BottomBlockedY - 20;
}

std::optional<Rect> geometry_switch_button_rect(const Rect& room_text_rect)
{
    if (is_room_row_blocked_by_bottom(room_text_rect)) {
        return std::nullopt;
    }
    return Rect(
        SwitchButtonLeftX,
        room_text_rect.y + SwitchButtonTopOffsetY,
        SwitchButtonWidth,
        SwitchButtonHeight);
}

constexpr int MaxMfgIndex = 5;
constexpr int MaxTradeIndex = 5;
constexpr int MaxPowerIndex = 3;

const std::vector<asst::InfrastPresetTask::RoomInfo>& all_room_infos()
{
    static const std::vector<asst::InfrastPresetTask::RoomInfo> Rooms = [] {
        std::vector<asst::InfrastPresetTask::RoomInfo> rooms = {
            { "Control", "控制中枢", 0 },
            { "Reception", "会客室", 1 },
        };
        int order = 2;
        for (int i = 1; i <= MaxMfgIndex; ++i) {
            rooms.emplace_back("Mfg" + std::to_string(i), "制造站" + std::to_string(i), order++);
        }
        for (int i = 1; i <= MaxTradeIndex; ++i) {
            rooms.emplace_back("Trade" + std::to_string(i), "贸易站" + std::to_string(i), order++);
        }
        for (int i = 1; i <= MaxPowerIndex; ++i) {
            rooms.emplace_back("Power" + std::to_string(i), "发电站" + std::to_string(i), order++);
        }
        rooms.emplace_back("Office", "办公室", order);
        return rooms;
    }();
    return Rooms;
}

std::string normalize_room_id(std::string id)
{
    std::ranges::transform(id, id.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

    static const std::unordered_map<std::string, std::string> Aliases = [] {
        std::unordered_map<std::string, std::string> aliases = {
            { "CONTROL", "Control" },
            { "RECEPTION", "Reception" },
            { "MEETING", "Reception" },
            { "OFFICE", "Office" },
        };
        for (int i = 1; i <= MaxMfgIndex; ++i) {
            const std::string index = std::to_string(i);
            aliases.emplace("MFG" + index, "Mfg" + index);
            aliases.emplace("MANUFACTURE" + index, "Mfg" + index);
        }
        for (int i = 1; i <= MaxTradeIndex; ++i) {
            const std::string index = std::to_string(i);
            aliases.emplace("TRADE" + index, "Trade" + index);
            aliases.emplace("TRADING" + index, "Trade" + index);
        }
        for (int i = 1; i <= MaxPowerIndex; ++i) {
            const std::string index = std::to_string(i);
            aliases.emplace("POWER" + index, "Power" + index);
        }
        return aliases;
    }();

    if (auto iter = Aliases.find(id); iter != Aliases.cend()) {
        return iter->second;
    }
    return {};
}

// 进驻总览列表里同类设施（制造站1/2/3/4 等）永远按编号自上而下连续排列，
// 因此无需“猜”被 OCR 丢失的数字，只要用同屏能认出数字的行作锚点即可推断。
struct NumberedTypeInfo
{
    std::string id_prefix;   // Mfg / Trade / Power
    std::string text_prefix; // 制造站 / 贸易站 / 发电站
    int max_index;
};

const std::vector<NumberedTypeInfo>& numbered_types()
{
    static const std::vector<NumberedTypeInfo> Types = {
        { "Mfg", "制造站", MaxMfgIndex },
        { "Trade", "贸易站", MaxTradeIndex },
        { "Power", "发电站", MaxPowerIndex },
    };
    return Types;
}

// 无编号的固定设施，OCR 一般稳定，直接精确匹配即可。
const std::vector<std::pair<std::string, std::string>>& fixed_rooms()
{
    static const std::vector<std::pair<std::string, std::string>> Rooms = {
        { "Control", "控制中枢" },
        { "Reception", "会客室" },
        { "Office", "办公室" },
    };
    return Rooms;
}

bool is_enabled_switch_pixel(const cv::Vec3b& pixel)
{
    const int blue = pixel[0];
    const int green = pixel[1];
    const int red = pixel[2];
    return blue > 100 && green > 80 && red < 80;
}

char normalize_ocr_digit(char ch)
{
    switch (ch) {
    case '1':
    case 'l':
    case 'I':
    case '|':
    case '!':
        return '1';
    case '2':
        return '2';
    case '3':
    case 'g':
    case 'G':
        return '3';
    case '4':
        return '4';
    case '5':
        return '5';
    case '6':
        return '6';
    case '7':
        return '7';
    case '8':
        return '8';
    case '9':
        return '9';
    default:
        return '\0';
    }
}

char normalize_ocr_digit(const std::string& text, size_t index)
{
    if (index >= text.size()) {
        return '\0';
    }

    const char digit = normalize_ocr_digit(text[index]);
    if (digit != '\0') {
        return digit;
    }

    if (text.compare(index, std::string("之").size(), "之") == 0) {
        return '2';
    }
    return '\0';
}

bool is_section_header_ocr_text(const std::string& text)
{
    auto is_ascii_bar = [](char ch) {
        return ch == '|' || ch == 'I' || ch == 'l' || ch == '!';
    };

    static const std::string FullwidthBar = "丨";
    static const std::vector<std::string> Prefixes = { "制造站", "贸易站", "发电站" };
    for (const auto& prefix : Prefixes) {
        const size_t pos = text.find(prefix);
        if (pos == std::string::npos) {
            continue;
        }
        const size_t after = pos + prefix.size();
        if (after < text.size() && normalize_ocr_digit(text, after) != '\0') {
            continue;
        }
        if (pos > 0) {
            const std::string_view head(text.data(), pos);
            if (std::ranges::any_of(head, is_ascii_bar) ||
                head.find(FullwidthBar) != std::string_view::npos) {
                return true;
            }
        }
        if (pos == 0 && !text.empty()) {
            if (is_ascii_bar(text[0]) || text.compare(0, FullwidthBar.size(), FullwidthBar) == 0) {
                return true;
            }
        }
    }

    static const std::vector<std::string> ExactHeaders = {
        "|制造站",
        "|贸易站",
        "|发电站",
        "丨制造站",
        "丨贸易站",
        "丨发电站",
    };
    return std::ranges::find(ExactHeaders, text) != ExactHeaders.cend();
}

enum class RoomRowKind
{
    None,     // 与目标无关的文本
    Fixed,    // 无编号固定设施（控制中枢/会客室/办公室）
    Numbered, // 同类编号设施且数字识别成功
    Bare,     // 同类编号设施但数字丢失（如“制造站”识别不到“3”）
};

struct RoomRowClass
{
    RoomRowKind kind = RoomRowKind::None;
    std::string id;   // Fixed 时为房间 id
    std::string type; // Numbered/Bare 时为类型前缀（Mfg/Trade/Power）
    int digit = 0;    // Numbered 时为编号
};

RoomRowClass classify_ocr_text(const std::string& text)
{
    if (is_section_header_ocr_text(text)) {
        return {};
    }

    for (const auto& [id, room_text] : fixed_rooms()) {
        if (text.find(room_text) != std::string::npos) {
            return RoomRowClass { RoomRowKind::Fixed, id, {}, 0 };
        }
    }

    for (const auto& type : numbered_types()) {
        const size_t prefix_pos = text.find(type.text_prefix);
        if (prefix_pos == std::string::npos) {
            continue;
        }
        const char digit = normalize_ocr_digit(text, prefix_pos + type.text_prefix.size());
        if (digit >= '1' && digit <= '0' + type.max_index) {
            return RoomRowClass { RoomRowKind::Numbered, {}, type.id_prefix, digit - '0' };
        }
        return RoomRowClass { RoomRowKind::Bare, {}, type.id_prefix, 0 };
    }

    return {};
}

struct RoomRow
{
    Rect rect;
    RoomRowClass cls;
    double score = 0.0;
};

// 同屏同类设施按 Y 连续排列，用能认出数字的行作锚点，确定性地推断其余行的编号。
void infer_type_rows(
    const NumberedTypeInfo& type,
    const std::vector<RoomRow>& rows_sorted_by_y,
    const std::unordered_set<std::string>& target_ids,
    std::unordered_map<std::string, Rect>& result)
{
    std::vector<const RoomRow*> type_rows;
    for (const auto& row : rows_sorted_by_y) {
        if ((row.cls.kind == RoomRowKind::Numbered || row.cls.kind == RoomRowKind::Bare) &&
            row.cls.type == type.id_prefix) {
            type_rows.emplace_back(&row);
        }
    }
    if (type_rows.empty()) {
        return;
    }

    // base = digit - position；每个锚点投一票，取得票最多的 base。
    std::unordered_map<int, int> base_votes;
    for (size_t pos = 0; pos < type_rows.size(); ++pos) {
        if (type_rows[pos]->cls.kind == RoomRowKind::Numbered) {
            base_votes[type_rows[pos]->cls.digit - static_cast<int>(pos)]++;
        }
    }

    int base = 0;
    if (!base_votes.empty()) {
        base = std::ranges::max_element(base_votes, {}, [](const auto& kv) { return kv.second; })->first;
    }
    else {
        // 整屏该类全部丢失数字：仅当“屏上恰好一行且只剩一个该类目标未处理”时才安全分配，否则放弃本屏重试。
        int pending_count = 0;
        std::string only_id;
        for (int idx = 1; idx <= type.max_index; ++idx) {
            const std::string id = type.id_prefix + std::to_string(idx);
            if (target_ids.contains(id) && !result.contains(id)) {
                ++pending_count;
                only_id = id;
            }
        }
        if (type_rows.size() == 1 && pending_count == 1) {
            result.emplace(only_id, type_rows.front()->rect);
            Log.trace("facility preset room inferred (single fallback):", only_id, type_rows.front()->rect.to_string());
        }
        return;
    }

    for (size_t pos = 0; pos < type_rows.size(); ++pos) {
        const int idx = base + static_cast<int>(pos);
        if (idx < 1 || idx > type.max_index) {
            continue;
        }
        const std::string id = type.id_prefix + std::to_string(idx);
        if (!target_ids.contains(id) || result.contains(id)) {
            continue;
        }
        result.emplace(id, type_rows[pos]->rect);
        Log.trace("facility preset room inferred:", id, type_rows[pos]->rect.to_string());
    }
}

std::optional<Rect> find_enabled_switch_button_by_color(const cv::Mat& image, const Rect& room_text_rect)
{
    if (is_room_row_blocked_by_bottom(room_text_rect)) {
        return std::nullopt;
    }

    const auto [y_begin, y_end] = switch_button_search_y_range(room_text_rect);
    const int x_begin = SwitchButtonSearchRoi.x;
    const int x_end = std::min(SwitchButtonSearchRoi.x + SwitchButtonSearchRoi.width, image.cols);

    if (y_begin >= y_end || x_begin >= x_end || y_begin >= image.rows) {
        return std::nullopt;
    }

    struct BlueRun
    {
        int y_begin = 0;
        int y_end = 0;
        int min_x = 0;
        int max_x = 0;
        int count = 0;
    };

    std::vector<BlueRun> runs;
    std::optional<BlueRun> cur_run;

    for (int y = y_begin; y < std::min(y_end, image.rows); ++y) {
        int row_min_x = image.cols;
        int row_max_x = 0;
        int row_count = 0;
        for (int x = x_begin; x < x_end; ++x) {
            if (!is_enabled_switch_pixel(image.at<cv::Vec3b>(y, x))) {
                continue;
            }
            row_min_x = std::min(row_min_x, x);
            row_max_x = std::max(row_max_x, x);
            ++row_count;
        }

        if (row_count >= 3) {
            if (!cur_run) {
                cur_run = BlueRun { .y_begin = y, .y_end = y, .min_x = row_min_x, .max_x = row_max_x, .count = row_count };
            }
            else {
                cur_run->y_end = y;
                cur_run->min_x = std::min(cur_run->min_x, row_min_x);
                cur_run->max_x = std::max(cur_run->max_x, row_max_x);
                cur_run->count += row_count;
            }
        }
        else if (cur_run) {
            runs.emplace_back(*cur_run);
            cur_run = std::nullopt;
        }
    }
    if (cur_run) {
        runs.emplace_back(*cur_run);
    }

    if (runs.empty()) {
        return std::nullopt;
    }

    const int expected_y = switch_button_expected_y(room_text_rect);
    auto best_iter = std::ranges::min_element(runs, [&](const BlueRun& lhs, const BlueRun& rhs) {
        const int lhs_center = (lhs.y_begin + lhs.y_end) / 2;
        const int rhs_center = (rhs.y_begin + rhs.y_end) / 2;
        return std::abs(lhs_center - expected_y) < std::abs(rhs_center - expected_y);
    });
    if (best_iter == runs.cend() || best_iter->count < 80 || best_iter->y_end > BottomBlockedY ||
        best_iter->y_end - best_iter->y_begin + 1 < SwitchButtonMinHeight) {
        return std::nullopt;
    }

    const int center_y = (best_iter->y_begin + best_iter->y_end) / 2;
    if (std::abs(center_y - expected_y) > SwitchButtonMaxYDrift) {
        return std::nullopt;
    }

    return Rect(best_iter->min_x, best_iter->y_begin, best_iter->max_x - best_iter->min_x + 1, best_iter->y_end - best_iter->y_begin + 1);
}
} // namespace

asst::InfrastPresetTask::InfrastPresetTask(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    AbstractTask(callback, inst, task_chain)
{
}

asst::InfrastPresetTask& asst::InfrastPresetTask::set_rooms(std::vector<std::string> rooms) noexcept
{
    m_rooms = std::move(rooms);
    return *this;
}

asst::InfrastPresetTask& asst::InfrastPresetTask::set_rest(bool rest) noexcept
{
    m_rest = rest;
    return *this;
}

bool asst::InfrastPresetTask::_run()
{
    LogTraceFunction;

    auto rooms = normalized_rooms();
    if (rooms.empty() && !m_rooms.empty()) {
        return false;
    }
    if (rooms.empty()) {
        Log.warn("facility preset rooms is empty, skip preset page");
        return true;
    }

    if (!ProcessTask(*this, { "InfrastEnterPresetPage" }).run()) {
        return false;
    }
    sleep(500);

    if (!click_preset_buttons(std::move(rooms))) {
        return false;
    }

    if (m_rest && !ProcessTask(*this, { "InfrastEnterTrim" }).run()) {
        return false;
    }

    if (!m_rest) {
        exit_preset_page();
    }

    return true;
}

bool asst::InfrastPresetTask::on_run_fails()
{
    LogTraceFunction;

    ProcessTask recover(*this, { "InfrastRotationReturn", "Infrast@ReturnButton" });
    recover.set_retry_times(3);
    recover.set_ignore_error(true);
    recover.run();

    return true;
}

void asst::InfrastPresetTask::exit_preset_page() const
{
    ProcessTask task(*this, { "InfrastRotationReturn" });
    task.set_retry_times(3);
    task.set_ignore_error(true);
    task.run();
}

std::vector<asst::InfrastPresetTask::RoomInfo> asst::InfrastPresetTask::normalized_rooms() const
{
    std::vector<RoomInfo> rooms;
    std::unordered_set<std::string> used;
    bool has_unknown_room = false;

    for (const auto& raw : m_rooms) {
        const std::string id = normalize_room_id(raw);
        if (id.empty()) {
            Log.error("unknown facility preset room:", raw);
            has_unknown_room = true;
            continue;
        }
        if (used.contains(id)) {
            continue;
        }
        used.emplace(id);

        auto iter = std::ranges::find_if(all_room_infos(), [&](const RoomInfo& info) { return info.id == id; });
        if (iter != all_room_infos().cend()) {
            rooms.emplace_back(*iter);
        }
    }

    if (has_unknown_room) {
        return {};
    }

    std::ranges::sort(rooms, {}, &RoomInfo::order);
    return rooms;
}

bool asst::InfrastPresetTask::click_preset_buttons(std::vector<RoomInfo> rooms)
{
    LogTraceFunction;

    for (int swipe_times = 0; !rooms.empty() && swipe_times <= MaxScrollTimes; ++swipe_times) {
        if (need_exit()) {
            return false;
        }

        const cv::Mat image = ctrler()->get_image();
        if (image.empty()) {
            Log.warn("facility preset empty screenshot");
            swipe_down();
            continue;
        }

        bool clicked = false;
        bool blocked_by_bottom = false;

        for (auto iter = rooms.begin(); iter != rooms.end();) {
            const cv::Mat room_image = ctrler()->get_image();
            if (room_image.empty()) {
                Log.warn("facility preset empty screenshot before click:", iter->id);
                ++iter;
                continue;
            }

            const auto current_visible = analyze_visible_rooms(room_image, rooms);
            const auto visible_iter = current_visible.find(iter->id);
            if (visible_iter == current_visible.cend()) {
                ++iter;
                continue;
            }

            auto button = find_enabled_switch_button(room_image, visible_iter->second);
            if (!button) {
                if (is_room_row_blocked_by_bottom(visible_iter->second)) {
                    blocked_by_bottom = true;
                    ++iter;
                    continue;
                }
                Log.warn("facility preset switch button not found, retry later:", iter->id, visible_iter->second.to_string());
                ++iter;
                continue;
            }

            Log.info("click facility preset switch:", iter->id, button->to_string());
            ctrler()->click(Point(
                button->x + button->width / 2 + SwitchButtonClickOffsetX,
                button->y + button->height / 2));
            sleep(800);
            clicked = true;
            iter = rooms.erase(iter);
        }

        if (rooms.empty()) {
            return true;
        }

        if (!clicked || blocked_by_bottom) {
            swipe_down();
        }
    }

    std::vector<std::string> remain;
    std::ranges::transform(rooms, std::back_inserter(remain), &RoomInfo::id);
    Log.error("facility preset rooms not found:", remain);
    save_img(utils::path("debug") / utils::path("infrast") / utils::path("preset"));
    return false;
}

std::unordered_map<std::string, Rect>
    asst::InfrastPresetTask::analyze_visible_rooms(const cv::Mat& image, const std::vector<RoomInfo>& rooms) const
{
    std::unordered_map<std::string, Rect> result;

    std::unordered_set<std::string> target_ids;
    for (const auto& room : rooms) {
        target_ids.insert(room.id);
    }

    OCRer analyzer(image);
    analyzer.set_roi(WorkAreaRoomTextRoi);
    const auto ocr_result = analyzer.analyze();
    if (!ocr_result) {
        return result;
    }

    // 收集所有可分类的行，并按 Y 聚类去重（同一张设施卡片可能产生多行 OCR）。
    std::vector<RoomRow> raw_rows;
    for (const auto& text_rect : *ocr_result) {
        auto cls = classify_ocr_text(text_rect.text);
        if (cls.kind == RoomRowKind::None) {
            continue;
        }
        raw_rows.emplace_back(RoomRow { text_rect.rect, std::move(cls), text_rect.score });
    }
    std::ranges::sort(raw_rows, {}, [](const RoomRow& row) { return row.rect.y; });

    constexpr int RowClusterY = 40;
    auto row_priority = [](const RoomRow& row) {
        return row.cls.kind == RoomRowKind::Bare ? 0 : 1;
    };

    std::vector<RoomRow> rows;
    for (auto& row : raw_rows) {
        if (!rows.empty() && std::abs(row.rect.y - rows.back().rect.y) < RowClusterY) {
            const bool better = std::pair(row_priority(row), row.score) >
                                std::pair(row_priority(rows.back()), rows.back().score);
            if (better) {
                rows.back() = row;
            }
            continue;
        }
        rows.emplace_back(row);
    }

    // 固定设施同屏常有两行 OCR：区块标题 + 实际预设行。取 Y 更大的一行，避免点到上一区块的按钮。
    std::unordered_map<std::string, Rect> fixed_room_rects;
    for (const auto& row : rows) {
        if (row.cls.kind != RoomRowKind::Fixed || !target_ids.contains(row.cls.id)) {
            continue;
        }
        const auto iter = fixed_room_rects.find(row.cls.id);
        if (iter == fixed_room_rects.end() || row.rect.y > iter->second.y) {
            fixed_room_rects[row.cls.id] = row.rect;
        }
    }
    for (const auto& [id, rect] : fixed_room_rects) {
        result.emplace(id, rect);
        Log.trace("facility preset room visible:", id, rect.to_string());
    }

    for (const auto& type : numbered_types()) {
        infer_type_rows(type, rows, target_ids, result);
    }

    return result;
}

std::optional<Rect> asst::InfrastPresetTask::find_enabled_switch_button(const cv::Mat& image, const Rect& room_text_rect) const
{
    if (image.empty() || image.channels() != 3) {
        return std::nullopt;
    }

    if (is_room_row_blocked_by_bottom(room_text_rect)) {
        return std::nullopt;
    }

    const auto [y_begin, y_end] = switch_button_search_y_range(room_text_rect);
    if (y_begin >= y_end) {
        return std::nullopt;
    }

    const Rect search_roi(
        SwitchButtonSearchRoi.x,
        y_begin,
        SwitchButtonSearchRoi.width,
        y_end - y_begin);

    const int expected_y = switch_button_expected_y(room_text_rect);

    auto accept_button = [&](const Rect& button) -> std::optional<Rect> {
        const int center_y = button.y + button.height / 2;
        if (std::abs(center_y - expected_y) > SwitchButtonMaxYDrift || button.y + button.height > BottomBlockedY) {
            return std::nullopt;
        }
        return button;
    };

    auto match_switch_in_roi = [&](const Rect& roi) -> std::optional<Rect> {
        asst::MultiMatcher roi_matcher(image);
        roi_matcher.set_task_info("InfrastPresetSwitchButton");
        roi_matcher.set_roi(roi);
        if (!roi_matcher.analyze() || roi_matcher.get_result().empty()) {
            return std::nullopt;
        }
        const auto& matches = roi_matcher.get_result();
        auto best_iter = std::ranges::min_element(matches, [&](const asst::MatchRect& lhs, const asst::MatchRect& rhs) {
            const int lhs_center = lhs.rect.y + lhs.rect.height / 2;
            const int rhs_center = rhs.rect.y + rhs.rect.height / 2;
            return std::abs(lhs_center - expected_y) < std::abs(rhs_center - expected_y);
        });
        if (best_iter == matches.cend()) {
            return std::nullopt;
        }
        if (auto button = accept_button(best_iter->rect)) {
            Log.trace("facility preset switch matched:", best_iter->to_string());
            return button;
        }
        return std::nullopt;
    };

    if (auto button = match_switch_in_roi(search_roi)) {
        return button;
    }

    if (auto geometry_button = geometry_switch_button_rect(room_text_rect)) {
        const Rect verify_roi(
            geometry_button->x - 8,
            geometry_button->y - 8,
            geometry_button->width + 16,
            geometry_button->height + 16);
        asst::MultiMatcher verify_matcher(image);
        verify_matcher.set_task_info("InfrastPresetSwitchButton");
        verify_matcher.set_roi(verify_roi);
        if (verify_matcher.analyze() && !verify_matcher.get_result().empty()) {
            const auto& matches = verify_matcher.get_result();
            const auto best_iter = std::ranges::max_element(matches, {}, &asst::MatchRect::score);
            if (best_iter != matches.cend() && best_iter->score >= SwitchButtonGeometryVerifyThreshold) {
                if (auto button = accept_button(best_iter->rect)) {
                    Log.trace(
                        "facility preset switch geometry verified:",
                        best_iter->score,
                        button->to_string());
                    return button;
                }
            }
        }
        if (auto button = accept_button(*geometry_button)) {
            Log.trace("facility preset switch geometry fallback:", button->to_string());
            return button;
        }
    }

    Log.trace("facility preset switch template miss, fallback to color scan");
    if (auto color_button = find_enabled_switch_button_by_color(image, room_text_rect)) {
        return accept_button(*color_button);
    }
    return std::nullopt;
}

void asst::InfrastPresetTask::swipe_down() const
{
    ctrler()->swipe(Point(700, 560), Point(700, 360), 300);
    sleep(600);
}
