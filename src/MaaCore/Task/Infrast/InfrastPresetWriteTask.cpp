#include "InfrastPresetWriteTask.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"
#include <algorithm>
#include <map>
#include <regex>
#include <set>

// Coordinates refer to the 1280x720 image supplied by Controller, never device pixels.
// All geometry actions below require a fresh, fully recognized preset dialog.
bool asst::InfrastPresetWriteTask::configure(const json::value& params)
{
    const std::map<std::string, size_t> capacities { { "control", 5 }, { "meeting", 2 }, { "manufacture", 3 },
                                                     { "trading", 3 }, { "power", 1 },   { "hire", 1 },
                                                     { "recycle", 2 } };
    auto rooms = params.find<json::array>("rooms");
    if (!rooms || rooms->empty() || rooms->size() > 20) {
        return false;
    }
    std::vector<Room> parsed;
    std::set<std::pair<std::string, int>> room_ids;
    std::array<std::set<std::string>, 3> used_names;
    for (const auto& entry : *rooms) {
        if (!entry.is_object()) {
            return false;
        }
        auto facility = entry.find<std::string>("facility");
        auto index = entry.find<int>("index");
        auto queues = entry.find<json::array>("queues");
        if (!facility || !capacities.contains(*facility) || !index || *index < 0 || *index > 8 || !queues ||
            queues->size() != 3 || !room_ids.emplace(*facility, *index).second) {
            return false;
        }
        Room room;
        room.facility = *facility;
        room.index = *index;
        for (size_t q = 0; q < 3; ++q) {
            const auto& names = queues->at(q);
            if (!names.is_array() || names.as_array().size() > capacities.at(*facility)) {
                return false;
            }
            for (const auto& name : names.as_array()) {
                if (!name.is_string() || name.as_string().empty() || !used_names[q].emplace(name.as_string()).second) {
                    return false;
                }
                room.queues[q].emplace_back(name.as_string());
            }
        }
        parsed.emplace_back(std::move(room));
    }
    if (std::ranges::all_of(used_names, [](const auto& names) { return names.empty(); })) {
        return false;
    }
    m_rooms = std::move(parsed);
    return true;
}

std::optional<asst::InfrastPresetWriteTask::Page> asst::InfrastPresetWriteTask::read_page() const
{
    if (need_exit()) {
        return std::nullopt;
    }
    const auto image = ctrler()->get_image();
    OCRer title(image, Rect(250, 40, 460, 55));
    OCRer number(image, Rect(940, 40, 140, 55));
    OCRer count(image, Rect(255, 302, 310, 36));
    auto titles = title.analyze();
    auto numbers = number.analyze();
    auto counts = count.analyze();
    if (!titles || titles->empty() || !numbers || numbers->size() != 1 || !counts || counts->size() != 1 ||
        titles->front().score < 0.85 || numbers->front().score < 0.85 || counts->front().score < 0.85) {
        return std::nullopt;
    }
    auto compact = [](std::string text) {
        std::erase_if(text, [](unsigned char c) { return c == ' ' || c == '\t' || c == '\n'; });
        return text;
    };
    Page page;
    std::ranges::sort(*titles, {}, [](const auto& result) { return result.rect.x; });
    page.title = compact(titles->front().text);
    const std::map<std::string, std::string> facilities { { "控制中枢", "control" },   { "会客室", "meeting" },
                                                          { "制造站", "manufacture" }, { "贸易站", "trading" },
                                                          { "发电站", "power" },       { "办公室", "hire" },
                                                          { "回收站", "recycle" } };
    for (const auto& [name, facility] : facilities) {
        if (page.title.starts_with(name)) {
            const auto suffix = page.title.substr(name.size());
            if (suffix.empty() || (suffix[0] >= '1' && suffix[0] <= '9')) {
                page.facility = facility;
                // The product name may be merged into the same OCR result (e.g. manufacture 1 gold).
                page.title = name + (suffix.empty() ? std::string() : suffix.substr(0, 1));
            }
        }
    }
    std::smatch match;
    const auto number_text = compact(numbers->front().text);
    if (!std::regex_match(number_text, match, std::regex("房间([1-9][0-9]?)/([1-9][0-9]?)"))) {
        return std::nullopt;
    }
    page.number = std::stoi(match[1].str());
    page.total = std::stoi(match[2].str());
    const auto count_text = compact(counts->front().text);
    if (!std::regex_match(count_text, match, std::regex("预设队列(?:\\(|（)([0-3])/3(?:\\)|）)"))) {
        return std::nullopt;
    }
    page.count = std::stoi(match[1].str());
    if (page.facility.empty() || page.total > 20 || page.number > page.total) {
        return std::nullopt;
    }
    return page;
}

bool asst::InfrastPresetWriteTask::same_room(const Page& expected) const
{
    const auto page = read_page();
    return page && page->number == expected.number && page->total == expected.total && page->title == expected.title;
}

bool asst::InfrastPresetWriteTask::move_room(const Page& page, int direction)
{
    const int target = page.number + direction;
    if (target < 1 || target > page.total || !same_room(page)) {
        return false;
    }
    ctrler()->click(Point(direction > 0 ? 1146 : 183, 336));
    sleep(700);
    const auto next = read_page();
    return next && next->number == target && next->total == page.total;
}

bool asst::InfrastPresetWriteTask::scroll_presets(const Page& page, bool down)
{
    if (!same_room(page)) {
        return false;
    }
    ctrler()->swipe(Point(790, down ? 610 : 380), Point(790, down ? 380 : 610), 400);
    sleep(650);
    return same_room(page);
}

std::optional<asst::Rect> asst::InfrastPresetWriteTask::find_text(const Rect& roi, const std::string& text) const
{
    OCRer ocr(ctrler()->get_image(), roi);
    const auto results = ocr.analyze();
    if (!results) {
        return std::nullopt;
    }
    std::optional<Rect> found;
    for (auto result : *results) {
        std::erase(result.text, ' ');
        if (result.text == text && result.score >= 0.85) {
            if (found) {
                return std::nullopt;
            }
            found = result.rect;
        }
    }
    return found;
}

bool asst::InfrastPresetWriteTask::ensure_queue(const Page& page, int queue)
{
    for (int attempt = 0; attempt < 4; ++attempt) {
        const auto current = read_page();
        if (!current || !same_room(page)) {
            return false;
        }
        if (current->count >= queue) {
            return true;
        }
        bool added = false;
        for (int swipe = 0; swipe < 5; ++swipe) {
            auto add = find_text(Rect(460, 350, 430, 290), "新增预设工作队列");
            if (add && same_room(page)) {
                ctrler()->click(*add);
                sleep(800);
                auto after = read_page();
                if (!after || after->number != page.number || after->title != page.title ||
                    after->count != current->count + 1) {
                    return false;
                }
                added = true;
                break;
            }
            if (!scroll_presets(page, true)) {
                return false;
            }
        }
        if (!added) {
            return false;
        }
    }
    return false;
}

bool asst::InfrastPresetWriteTask::open_queue(const Page& page, int queue)
{
    // Use the actual label at the current scroll position. Never infer a queue number from its row.
    for (int swipe = 0; swipe < 5; ++swipe) {
        auto label = find_text(Rect(265, 347, 145, 248), "队列" + std::to_string(queue));
        if (label && label->y + label->height / 2 + 46 < 635 && same_room(page)) {
            // Portrait area only: current-stationed row, orange delete and blue apply are outside this rectangle.
            // Only actual operator slots open the editor; later slots are disabled in smaller rooms.
            ctrler()->click(Point(510, label->y + label->height / 2 + 46));
            sleep(800);
            if (!find_text(Rect(930, 630, 330, 90), "确认")) {
                return false;
            }
            // Single-seat editors show room/operator details instead of the multi-select clear button.
            return m_capacity == 1 ? find_text(Rect(20, 85, 360, 50), page.title).has_value()
                                   : find_text(Rect(370, 630, 270, 90), "清空选择").has_value();
        }
        if (!scroll_presets(page, queue == 3 || (queue == 2 && swipe >= 2))) {
            return false;
        }
    }
    return false;
}

bool asst::InfrastPresetWriteTask::verify_selection(const std::vector<std::string>& names)
{
    if (need_exit()) {
        return false;
    }
    // Sorting brings all selected operators to the first page; it does not change selection.
    if (!ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked", "Stop" }).run()) {
        return false;
    }
    swipe_to_the_left_of_operlist();
    sleep(500);
    const auto image = ctrler()->get_image();
    InfrastOperImageAnalyzer analyzer(image);
    analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::Selected);
    if (!analyzer.analyze()) {
        return false;
    }
    std::vector<std::string> selected(names.size());
    size_t count = 0;
    const auto replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    for (const auto& oper : analyzer.get_result()) {
        if (!oper.selected) {
            continue;
        }
        ++count;
        RegionOCRer ocr;
        ocr.set_replace(replace->replace_map, replace->replace_full);
        ocr.set_image(oper.name_img);
        ocr.set_bin_expansion(0);
        if (!ocr.analyze()) {
            return false;
        }
        size_t index = 0;
        if (m_capacity > 1) {
            // Read the selection badge, not the card's display position or analyzer traversal order.
            OCRer order(image, oper.rect.move(Rect(65, 5, 55, 70)));
            const auto results = order.analyze();
            if (!results || results->size() != 1 || results->front().score < 0.85) {
                LogWarn << __FUNCTION__ << "Could not recognize selection order" << ocr.get_result().text;
                return false;
            }
            const auto& text = results->front().text;
            if (text.size() != 1 || text[0] < '1' || text[0] > '5') {
                LogWarn << __FUNCTION__ << "Invalid selection order" << text;
                return false;
            }
            index = static_cast<size_t>(text[0] - '1');
        }
        // Single-seat editors have no order badge. Multiple cards or duplicate badges still fail.
        if (index >= selected.size() || !selected[index].empty()) {
            LogWarn << __FUNCTION__ << "Duplicate or unexpected selection position" << index + 1;
            return false;
        }
        selected[index] = ocr.get_result().text;
    }
    return count == names.size() && selected == names;
}

bool asst::InfrastPresetWriteTask::write_queue(const Page& page, int queue, const std::vector<std::string>& names)
{
    if (!ensure_queue(page, queue) || !open_queue(page, queue)) {
        return false;
    }
    // Re-imports can resume after a partial failure without rewriting matching saved rosters.
    if (verify_selection(names)) {
        if (!ProcessTask(*this, { "InfrastPresetImportBack" }).run() || !same_room(page)) {
            return false;
        }
        ++m_verified;
        report("verified", page.title, queue);
        return true;
    }
    infrast::CustomRoomConfig config;
    config.names = names;
    config.sort = true;
    set_custom_config({ config });
    m_cur_facility_index = 0;
    // Single-seat editors replace the current choice on click and have no clear button.
    if ((m_capacity > 1 && !click_clear_button()) || !swipe_and_select_custom_opers(false, true) ||
        !verify_selection(names)) {
        return false;
    }
    if (!ProcessTask(*this, { "InfrastPresetImportConfirm" }).run() || !same_room(page) || !open_queue(page, queue) ||
        !verify_selection(names)) {
        return false;
    }
    // Readback exits without saving a second time. No actual staffing confirmation chain is called.
    if (!ProcessTask(*this, { "InfrastPresetImportBack" }).run() || !same_room(page)) {
        return false;
    }
    ++m_verified;
    report("verified", page.title, queue);
    return true;
}

void asst::InfrastPresetWriteTask::report(const std::string& stage, const std::string& room, int queue)
{
    auto info = basic_info_with_what("InfrastPresetImportProgress");
    info["details"]["stage"] = stage;
    info["details"]["room"] = room;
    info["details"]["queue"] = queue;
    info["details"]["verified"] = m_verified;
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::InfrastPresetWriteTask::_run()
{
    m_verified = 0;
    m_current_room.clear();
    m_current_queue = 0;
    if (!ProcessTask(*this, { "InfrastPresetImportBegin" }).set_retry_times(3).run()) {
        return false;
    }
    auto page = read_page();
    if (!page) {
        return false;
    }
    while (page->number > 1) {
        if (!move_room(*page, -1) || !(page = read_page())) {
            return false;
        }
    }
    // Read the full room topology before changing any preset. Array indices follow in-game facility order.
    std::vector<Page> pages;
    std::map<std::pair<std::string, int>, int> positions;
    std::map<std::string, int> counts;
    for (;;) {
        m_current_room = page->title;
        positions[{ page->facility, counts[page->facility]++ }] = page->number;
        pages.emplace_back(*page);
        if (page->number == page->total) {
            break;
        }
        if (!move_room(*page, 1) || !(page = read_page())) {
            return false;
        }
    }
    for (const auto& room : m_rooms) {
        if (!positions.contains({ room.facility, room.index })) {
            LogError << "Preset import room does not exist" << room.facility << room.index;
            return false;
        }
    }
    // Creating queue 3 also creates earlier slots. Refuse to synthesize a roster for a skipped slot.
    for (const auto& room : m_rooms) {
        const auto& existing = pages.at(positions.at({ room.facility, room.index }) - 1);
        int last = 0;
        for (int q = 0; q < 3; ++q) {
            if (!room.queues[q].empty()) {
                last = q + 1;
            }
        }
        for (int q = existing.count; q < last; ++q) {
            if (room.queues[q].empty()) {
                LogError << "Missing earlier preset has no roster" << existing.title << q + 1;
                m_current_room = existing.title;
                m_current_queue = q + 1;
                return false;
            }
        }
    }
    while (page->number > 1) {
        if (!move_room(*page, -1) || !(page = read_page())) {
            return false;
        }
    }
    const std::map<std::string, size_t> capacities { { "control", 5 }, { "meeting", 2 }, { "manufacture", 3 },
                                                     { "trading", 3 }, { "power", 1 },   { "hire", 1 },
                                                     { "recycle", 2 } };
    for (const auto& expected : pages) {
        if (!same_room(expected)) {
            return false;
        }
        m_current_room = expected.title;
        for (const auto& room : m_rooms) {
            if (positions.at({ room.facility, room.index }) != expected.number) {
                continue;
            }
            m_facility = room.facility;
            m_capacity = capacities.at(room.facility);
            for (int q = 1; q <= 3; ++q) {
                m_current_queue = q;
                if (room.queues[q - 1].empty()) {
                    continue;
                }
                report("editing", expected.title, q);
                if (!write_queue(expected, q, room.queues[q - 1])) {
                    return false;
                }
            }
        }
        if (expected.number < expected.total && !move_room(expected, 1)) {
            return false;
        }
    }
    report("completed");
    return true;
}

bool asst::InfrastPresetWriteTask::on_run_fails()
{
    // Preserve the exact failed page for diagnosis; never recover via the normal staffing/rotation chain.
    save_img(utils::path("debug") / utils::path("infrast-preset-import"));
    report("failed", m_current_room, m_current_queue);
    return false;
}
