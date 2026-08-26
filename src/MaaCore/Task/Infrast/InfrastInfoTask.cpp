#include "InfrastInfoTask.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastFacilityImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"

namespace
{
using ViewType = asst::InfrastFacilityImageAnalyzer::ViewType;

constexpr double NormalLevelTemplateScale = 1.45;
constexpr int StationMaxLevel = 3;
constexpr int DormMaxLevel = 5;
constexpr size_t MiniStationRowCount = 3;
constexpr size_t MiniStationColumnCount = 3;
constexpr size_t MiniDormRowCount = 4;

std::string_view level_template_task_name(std::string_view facility)
{
    if (facility == "Mfg") {
        return "InfrastInfoMfgLevelMini";
    }
    if (facility == "Trade") {
        return "InfrastInfoTradeLevelMini";
    }
    if (facility == "Power") {
        return "InfrastInfoPowerLevelMini";
    }
    return "InfrastInfoDormLevelMini";
}

std::string mini_station_level_task_name(size_t row, size_t column, int level)
{
    return "InfrastInfoMiniStationRow" + std::to_string(row) + "Column" + std::to_string(column) + "Level" +
           std::to_string(level);
}

std::string mini_dorm_level_task_name(size_t row, int level)
{
    return "InfrastInfoMiniDormRow" + std::to_string(row) + "Level" + std::to_string(level);
}

std::string mini_right_level_task_name(std::string_view facility, int level)
{
    return "InfrastInfoMini" + std::string(facility) + "Level" + std::to_string(level);
}

template <typename TaskNameFactory>
int recognize_highest_mini_level(
    const cv::Mat& image,
    std::string_view template_task_name,
    int max_level,
    TaskNameFactory&& task_name_factory,
    const std::optional<asst::Rect>& relative_to = std::nullopt)
{
    // 每个资源任务只覆盖一个等级标记，从最右侧最高级向左返回首个命中。
    const auto template_task = asst::Task.get<asst::MatchTaskInfo>(template_task_name);
    for (int candidate_level = max_level; candidate_level >= 1; --candidate_level) {
        const auto roi_task = asst::Task.get(task_name_factory(candidate_level));
        const auto roi = relative_to ? relative_to->move(roi_task->roi) : roi_task->roi;
        asst::Matcher matcher(image);
        matcher.set_task_info(template_task);
        matcher.set_roi(roi);
        if (matcher.analyze()) {
            return candidate_level;
        }
    }
    return 0;
}

asst::Point center_of(const asst::Rect& rect)
{
    return { rect.x + rect.width / 2, rect.y + rect.height / 2 };
}

int squared_distance(const asst::Point& lhs, const asst::Point& rhs)
{
    const int dx = lhs.x - rhs.x;
    const int dy = lhs.y - rhs.y;
    return dx * dx + dy * dy;
}

int level_or_maximum(int recognized_level, std::string_view facility, int max_level, const asst::Rect& facility_rect)
{
    if (recognized_level > 0 && recognized_level <= max_level) {
        return recognized_level;
    }

    Log.warn(
        "infrastructure level recognition failed, using maximum level",
        "facility",
        facility,
        "recognized level",
        recognized_level,
        "fallback level",
        max_level,
        "facility rect",
        facility_rect);
    return max_level;
}

int recognize_level(
    const cv::Mat& image,
    std::string_view task_name,
    const asst::Rect& roi,
    ViewType view_type,
    int max_level)
{
    const auto task = asst::Task.get<asst::MatchTaskInfo>(task_name);
    asst::MultiMatcher matcher(image);
    matcher.set_task_info(task);
    matcher.set_roi(roi);

    if (view_type == ViewType::Normal) {
        if (task->templ_names.empty()) {
            Log.error("missing infrastructure level template", task_name);
            return 0;
        }
        const auto& mini_templ = asst::TemplResource::get_instance().get_templ(task->templ_names.front());
        cv::Mat normal_templ;
        cv::resize(
            mini_templ,
            normal_templ,
            cv::Size(),
            NormalLevelTemplateScale,
            NormalLevelTemplateScale,
            cv::INTER_LINEAR);
        matcher.set_templ(std::move(normal_templ));
    }

    const auto result = matcher.analyze();
    const int level = result ? static_cast<int>(result->size()) : 0;
    if (level <= 0 || level > max_level) {
        Log.warn("invalid infrastructure level match count", task_name, level, "roi", roi);
        return 0;
    }
    return level;
}

bool recognize_mini_layout(
    const cv::Mat& image,
    const asst::InfrastFacilityImageAnalyzer& analyzer,
    std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities)
{
    // 设施分析器负责确定设施类别和数量，等级任务按槽位和等级分别提供单个标记 ROI。
    std::unordered_set<size_t> used_station_slots;
    for (const auto& facility_name : { "Mfg", "Trade", "Power" }) {
        const auto iter = analyzer.get_result().find(facility_name);
        if (iter == analyzer.get_result().end()) {
            continue;
        }
        for (const auto& match : iter->second) {
            size_t best_slot = MiniStationRowCount * MiniStationColumnCount;
            int best_distance = (std::numeric_limits<int>::max)();
            for (size_t index = 0; index < MiniStationRowCount * MiniStationColumnCount; ++index) {
                if (used_station_slots.contains(index)) {
                    continue;
                }
                const auto row = index / MiniStationColumnCount + 1;
                const auto column = index % MiniStationColumnCount + 1;
                const auto roi_task = asst::Task.get(mini_station_level_task_name(row, column, 1));
                const int distance = squared_distance(center_of(match.rect), center_of(roi_task->roi));
                if (distance < best_distance) {
                    best_slot = index;
                    best_distance = distance;
                }
            }
            if (best_slot == MiniStationRowCount * MiniStationColumnCount) {
                Log.warn("no free mini station level ROI", facility_name, match.rect);
                return false;
            }

            const auto row = best_slot / MiniStationColumnCount + 1;
            const auto column = best_slot % MiniStationColumnCount + 1;
            const int level = level_or_maximum(
                recognize_highest_mini_level(
                    image,
                    level_template_task_name(facility_name),
                    StationMaxLevel,
                    [row, column](int candidate_level) {
                        return mini_station_level_task_name(row, column, candidate_level);
                    }),
                facility_name,
                StationMaxLevel,
                match.rect);
            used_station_slots.emplace(best_slot);
            facilities[facility_name].push_back({ match.rect, level });
        }
    }

    const auto dorm_iter = analyzer.get_result().find("Dorm");
    if (dorm_iter != analyzer.get_result().end()) {
        std::unordered_set<size_t> used_slots;
        for (const auto& match : dorm_iter->second) {
            size_t best_slot = MiniDormRowCount;
            int best_distance = (std::numeric_limits<int>::max)();
            for (size_t index = 0; index < MiniDormRowCount; ++index) {
                if (used_slots.contains(index)) {
                    continue;
                }
                const auto roi_task = asst::Task.get(mini_dorm_level_task_name(index + 1, 1));
                const int distance = squared_distance(center_of(match.rect), center_of(roi_task->roi));
                if (distance < best_distance) {
                    best_slot = index;
                    best_distance = distance;
                }
            }
            if (best_slot == MiniDormRowCount) {
                Log.warn("no free mini dorm level ROI", match.rect);
                return false;
            }

            const int level = level_or_maximum(
                recognize_highest_mini_level(
                    image,
                    level_template_task_name("Dorm"),
                    DormMaxLevel,
                    [row = best_slot + 1](int candidate_level) {
                        return mini_dorm_level_task_name(row, candidate_level);
                    }),
                "Dorm",
                DormMaxLevel,
                match.rect);
            used_slots.emplace(best_slot);
            facilities["Dorm"].push_back({ match.rect, level });
        }
    }

    // 右侧设施的等级 ROI 相对于设施匹配矩形定义，同样从最高等级向左检查。
    for (const auto& facility_name : { "Reception", "Office", "Processing", "Training" }) {
        const auto iter = analyzer.get_result().find(std::string(facility_name));
        if (iter == analyzer.get_result().end()) {
            continue;
        }
        for (const auto& match : iter->second) {
            const int level = level_or_maximum(
                recognize_highest_mini_level(
                    image,
                    level_template_task_name("Dorm"),
                    StationMaxLevel,
                    [facility_name](int candidate_level) {
                        return mini_right_level_task_name(facility_name, candidate_level);
                    },
                    match.rect),
                facility_name,
                StationMaxLevel,
                match.rect);
            facilities[std::string(facility_name)].push_back({ match.rect, level });
        }
    }
    return true;
}

bool recognize_normal_layout(
    const cv::Mat& image,
    const asst::InfrastFacilityImageAnalyzer& analyzer,
    std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities)
{
    for (const auto& name : { "Mfg", "Trade", "Power" }) {
        const auto iter = analyzer.get_result().find(name);
        if (iter == analyzer.get_result().end()) {
            continue;
        }
        const auto task_name = level_template_task_name(name);
        const auto level_roi_move = asst::Task.get(task_name)->rect_move;
        for (const auto& match : iter->second) {
            const int level = level_or_maximum(
                recognize_level(image, task_name, match.rect.move(level_roi_move), ViewType::Normal, StationMaxLevel),
                name,
                StationMaxLevel,
                match.rect);
            facilities[name].push_back({ match.rect, level });
        }
    }

    const auto dorm_iter = analyzer.get_result().find("Dorm");
    if (dorm_iter == analyzer.get_result().end()) {
        return true;
    }
    const auto task_name = level_template_task_name("Dorm");
    const auto level_roi_move = asst::Task.get(task_name)->rect_move;
    for (const auto& match : dorm_iter->second) {
        const int level = level_or_maximum(
            recognize_level(image, task_name, match.rect.move(level_roi_move), ViewType::Normal, DormMaxLevel),
            "Dorm",
            DormMaxLevel,
            match.rect);
        facilities["Dorm"].push_back({ match.rect, level });
    }
    return true;
}

struct FacilityLayoutCounts
{
    size_t mfg = 0;
    size_t trade = 0;
    size_t power = 0;
    size_t dorm = 0;
    size_t control = 0;
    size_t reception = 0;
    size_t office = 0;
    size_t processing = 0;
    size_t training = 0;

    bool operator==(const FacilityLayoutCounts&) const = default;

    size_t production_count() const noexcept { return mfg + trade + power; }
};

FacilityLayoutCounts
    count_facilities(const std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities)
{
    const auto count = [&](std::string_view name) {
        const auto iter = facilities.find(std::string(name));
        return iter == facilities.end() ? size_t { 0 } : iter->second.size();
    };
    return {
        .mfg = count("Mfg"),
        .trade = count("Trade"),
        .power = count("Power"),
        .dorm = count("Dorm"),
        .control = count("Control"),
        .reception = count("Reception"),
        .office = count("Office"),
        .processing = count("Processing"),
        .training = count("Training"),
    };
}

bool is_usable_layout(const FacilityLayoutCounts& counts)
{
    const size_t production_count = counts.production_count();
    return production_count > 0 && production_count <= 9 && counts.dorm <= 4 && counts.reception <= 1 &&
           counts.office <= 1 && counts.processing <= 1 && counts.training <= 1;
}

bool is_complete_layout(const FacilityLayoutCounts& counts)
{
    return counts.production_count() == 9 && counts.mfg != 0 && counts.trade != 0 && counts.power != 0 &&
           counts.dorm == 4 && counts.reception == 1 && counts.office == 1;
}
} // namespace

bool asst::InfrastInfoTask::try_zoom_out()
{
    const auto pointer0 = Task.get("InfrastInfoZoomOutPointer0");
    const auto pointer1 = Task.get("InfrastInfoZoomOutPointer1");
    const Point pointer0_start { pointer0->specific_rect.x, pointer0->specific_rect.y };
    const Point pointer1_start { pointer1->specific_rect.x, pointer1->specific_rect.y };
    const Point pointer0_end { pointer0->rect_move.x, pointer0->rect_move.y };
    const Point pointer1_end { pointer1->rect_move.x, pointer1->rect_move.y };

    // 两指从画面两侧向中心收拢。旧 ADB 和 PlayTools 控制器不支持多指输入，
    // 此时立即返回，后续仍同时使用正常/最小视图模板识别。
    // MOVE 必须逐点插值并保持步间等待：没有中间轨迹的单步瞬移会被游戏端
    // 判成点击或页面滑动等独立手势，而不是捏合缩放。
    constexpr int ZoomSteps = 20;
    constexpr long StepDelayMs = 25;
    constexpr long HoldBeforeUpMs = 100;

    const auto lerp = [](const Point& from, const Point& to, double t) {
        return Point { static_cast<int>(std::lround(from.x * (1 - t) + to.x * t)),
                       static_cast<int>(std::lround(from.y * (1 - t) + to.y * t)) };
    };

    std::vector<InputEvent> events;
    events.reserve(3 + static_cast<size_t>(ZoomSteps) * 4 + 4);
    events.emplace_back(InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 0, .point = pointer0_start });
    events.emplace_back(InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 1, .point = pointer1_start });
    events.emplace_back(InputEvent { .type = InputEvent::Type::COMMIT });
    for (int step = 1; step <= ZoomSteps; ++step) {
        const double ratio = static_cast<double>(step) / ZoomSteps;
        events.emplace_back(InputEvent {
            .type = InputEvent::Type::TOUCH_MOVE,
            .pointerId = 0,
            .point = lerp(pointer0_start, pointer0_end, ratio),
        });
        events.emplace_back(InputEvent {
            .type = InputEvent::Type::TOUCH_MOVE,
            .pointerId = 1,
            .point = lerp(pointer1_start, pointer1_end, ratio),
        });
        events.emplace_back(InputEvent { .type = InputEvent::Type::COMMIT });
        events.emplace_back(InputEvent { .type = InputEvent::Type::WAIT_MS, .milisec = StepDelayMs });
    }
    events.emplace_back(InputEvent { .type = InputEvent::Type::WAIT_MS, .milisec = HoldBeforeUpMs });
    events.emplace_back(InputEvent { .type = InputEvent::Type::TOUCH_UP, .pointerId = 0 });
    events.emplace_back(InputEvent { .type = InputEvent::Type::TOUCH_UP, .pointerId = 1 });
    events.emplace_back(InputEvent { .type = InputEvent::Type::COMMIT });

    for (auto event : events) {
        if (!ctrler()->inject_input_event(event)) {
            InputEvent reset { .type = InputEvent::Type::TOUCH_RESET };
            ctrler()->inject_input_event(reset);
            return false;
        }
    }
    sleep(500);
    return true;
}

bool asst::InfrastInfoTask::_run()
{
    swipe_to_the_left_of_main_ui();

    if (!m_layout_required) {
        const auto image = ctrler()->get_image();
        InfrastFacilityImageAnalyzer analyzer(image);
        analyzer.set_to_be_analyzed({ "Mfg", "Trade", "Power", "Dorm" });
        if (!analyzer.analyze()) {
            return false;
        }
        for (const auto& [name, result] : analyzer.get_result()) {
            const std::string key = "NumOf" + name;
            status()->set_number(key, result.size());
            Log.trace("InfrastInfoTask | ", key, result.size());
        }
        return true;
    }

    const bool zoom_sent = try_zoom_out();
    Log.info("InfrastInfoTask | zoom gesture", zoom_sent ? "sent" : "unsupported");

    constexpr int MaxAttempts = 3;
    const auto prepare_retry = [&](int attempt) {
        // A pinch may advance only one zoom level. When the first gesture leaves
        // the overview at an intermediate scale, waiting cannot make the fixed-size
        // normal or mini templates match; pinch again before retrying.
        if (zoom_sent && attempt < MaxAttempts) {
            const bool retry_zoom_sent = try_zoom_out();
            Log.info(
                "InfrastInfoTask | retry zoom gesture",
                retry_zoom_sent ? "sent" : "unsupported",
                "after attempt",
                attempt);
            if (retry_zoom_sent) {
                return;
            }
        }
        sleep(300);
    };
    std::optional<FacilityLayoutCounts> partial_layout_candidate;
    for (int attempt = 1; attempt <= MaxAttempts; ++attempt) {
        if (need_exit()) {
            return false;
        }

        const auto image = ctrler()->get_image();
        InfrastFacilityImageAnalyzer analyzer(image);
        analyzer.set_to_be_analyzed(
            { "Mfg", "Trade", "Power", "Dorm", "Control", "Reception", "Office", "Processing", "Training" });
        if (!analyzer.analyze()) {
            partial_layout_candidate.reset();
            Log.warn("InfrastInfoTask | no facility matched, attempt", attempt);
            prepare_retry(attempt);
            continue;
        }

        std::unordered_map<std::string, std::vector<infrast::FacilityInfo>> facilities;
        const bool layout_mapped = analyzer.get_view_type() == ViewType::Mini
                                       ? recognize_mini_layout(image, analyzer, facilities)
                                       : recognize_normal_layout(image, analyzer, facilities);

        if (analyzer.get_view_type() != ViewType::Mini) {
            for (const auto& name : { "Reception", "Office", "Processing", "Training" }) {
                if (const auto iter = analyzer.get_result().find(name); iter != analyzer.get_result().end()) {
                    auto& output = facilities[name];
                    std::ranges::transform(iter->second, std::back_inserter(output), [](const MatchRect& match) {
                        return infrast::FacilityInfo { match.rect, 0 };
                    });
                }
            }
        }
        if (const auto iter = analyzer.get_result().find("Control"); iter != analyzer.get_result().end()) {
            auto& output = facilities["Control"];
            std::ranges::transform(iter->second, std::back_inserter(output), [](const MatchRect& match) {
                return infrast::FacilityInfo { match.rect, 0 };
            });
        }

        const auto layout_counts = count_facilities(facilities);
        if (!layout_mapped || !is_usable_layout(layout_counts)) {
            partial_layout_candidate.reset();
            Log.warn(
                "InfrastInfoTask | inconsistent facility layout, attempt",
                attempt,
                "view",
                static_cast<int>(analyzer.get_view_type()));
            prepare_retry(attempt);
            continue;
        }

        if (!is_complete_layout(layout_counts) && partial_layout_candidate != layout_counts) {
            partial_layout_candidate = layout_counts;
            Log.info("InfrastInfoTask | partial facility layout detected, confirming", attempt);
            sleep(300);
            continue;
        }

        m_task_data->facilities = std::move(facilities);
        const auto get_count = [&](std::string_view name) {
            return static_cast<int>(m_task_data->facilities[std::string(name)].size());
        };
        for (const auto& name : { "Mfg", "Trade", "Power", "Dorm" }) {
            const std::string key = "NumOf" + std::string(name);
            status()->set_number(key, get_count(name));
            Log.trace("InfrastInfoTask | ", key, get_count(name));
        }

        m_task_data->trading_station_num = get_count("Trade");
        m_task_data->power_station_num = get_count("Power");
        m_task_data->virtual_power_station_num = m_task_data->power_station_num;
        m_task_data->dormitory_capacity = get_count("Dorm") * 5;
        m_task_data->dormitory_level_sum = 0;
        for (const auto& dorm : m_task_data->facilities["Dorm"]) {
            m_task_data->dormitory_level_sum += dorm.level;
        }

        int recognized_level_sum = 0;
        for (const auto& [name, facility_list] : m_task_data->facilities) {
            if (name == "Control") {
                continue;
            }
            for (const auto& facility : facility_list) {
                recognized_level_sum += facility.level;
            }
        }
        // 控制中枢等级由宿舍数量推导。普通视图没有固定的右侧等级 ROI，
        // 因此保留原有的比例估算；最小视图则使用右侧资源 ROI 的实际识别结果。
        int estimated_right_level = 0;
        if (analyzer.get_view_type() == ViewType::Normal) {
            const int recognized_max_level = static_cast<int>(
                layout_counts.production_count() * StationMaxLevel + layout_counts.dorm * DormMaxLevel);
            const int right_facility_count = static_cast<int>(
                layout_counts.reception + layout_counts.office + layout_counts.processing + layout_counts.training);
            if (recognized_max_level > 0) {
                estimated_right_level = static_cast<int>(std::round(
                    recognized_level_sum * static_cast<double>(right_facility_count * StationMaxLevel) /
                    recognized_max_level));
            }
        }
        const int control_level = get_count("Dorm") + 1;
        m_task_data->total_station_level = recognized_level_sum + estimated_right_level + control_level;
        return true;
    }

    save_img(utils::path("debug") / utils::path("infrast") / utils::path("facility_layout"), false);
    json::value error = basic_info_with_what("FacilityLayoutRecognitionFailed");
    error["details"]["attempts"] = MaxAttempts;
    callback(AsstMsg::SubTaskError, error);
    Log.error("InfrastInfoTask | facility layout recognition failed after", MaxAttempts, "attempts");
    return false;
}
