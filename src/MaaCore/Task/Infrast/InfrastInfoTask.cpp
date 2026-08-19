#include "InfrastInfoTask.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastFacilityImageAnalyzer.h"
#include "Vision/MultiMatcher.h"

namespace
{
using ViewType = asst::InfrastFacilityImageAnalyzer::ViewType;

constexpr std::array<std::string_view, 9> MiniStationLevelRoiTasks = {
    "InfrastInfoMiniStation1Level", "InfrastInfoMiniStation2Level", "InfrastInfoMiniStation3Level",
    "InfrastInfoMiniStation4Level", "InfrastInfoMiniStation5Level", "InfrastInfoMiniStation6Level",
    "InfrastInfoMiniStation7Level", "InfrastInfoMiniStation8Level", "InfrastInfoMiniStation9Level"
};
constexpr std::array<std::string_view, 4> MiniDormLevelRoiTasks = {
    "InfrastInfoMiniDorm1Level",
    "InfrastInfoMiniDorm2Level",
    "InfrastInfoMiniDorm3Level",
    "InfrastInfoMiniDorm4Level",
};
constexpr std::array<std::pair<std::string_view, std::string_view>, 4> MiniRightLevelRoiTasks = {
    std::pair { "Reception", "InfrastInfoMiniReceptionLevel" },
    std::pair { "Office", "InfrastInfoMiniOfficeLevel" },
    std::pair { "Processing", "InfrastInfoMiniProcessingLevel" },
    std::pair { "Training", "InfrastInfoMiniTrainingLevel" },
};
constexpr double NormalLevelTemplateScale = 1.45;
constexpr int StationMaxLevel = 3;
constexpr int DormMaxLevel = 5;

std::string_view level_task_name(std::string_view facility)
{
    if (facility == "Mfg") {
        return "InfrastInfoMfgLevel";
    }
    if (facility == "Trade") {
        return "InfrastInfoTradeLevel";
    }
    if (facility == "Power") {
        return "InfrastInfoPowerLevel";
    }
    return "InfrastInfoDormLevel";
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
    // 设施分析器负责确定设施类别和数量，固定资源 ROI 只提供最小视图各槽位的等级标记区域。
    std::unordered_set<size_t> used_station_slots;
    for (const auto& facility_name : { "Mfg", "Trade", "Power" }) {
        const auto iter = analyzer.get_result().find(facility_name);
        if (iter == analyzer.get_result().end()) {
            continue;
        }
        for (const auto& match : iter->second) {
            size_t best_slot = MiniStationLevelRoiTasks.size();
            int best_distance = (std::numeric_limits<int>::max)();
            for (size_t index = 0; index < MiniStationLevelRoiTasks.size(); ++index) {
                if (used_station_slots.contains(index)) {
                    continue;
                }
                const auto roi_task = asst::Task.get(MiniStationLevelRoiTasks[index]);
                const int distance = squared_distance(center_of(match.rect), center_of(roi_task->roi));
                if (distance < best_distance) {
                    best_slot = index;
                    best_distance = distance;
                }
            }
            if (best_slot == MiniStationLevelRoiTasks.size()) {
                Log.warn("no free mini station level ROI", facility_name, match.rect);
                return false;
            }

            const auto roi_task = asst::Task.get(MiniStationLevelRoiTasks[best_slot]);
            const int level = recognize_level(
                image,
                level_task_name(facility_name),
                roi_task->roi,
                ViewType::Mini,
                StationMaxLevel);
            if (level == 0) {
                return false;
            }
            used_station_slots.emplace(best_slot);
            facilities[facility_name].push_back({ match.rect, level });
        }
    }

    const auto dorm_iter = analyzer.get_result().find("Dorm");
    if (dorm_iter != analyzer.get_result().end()) {
        std::unordered_set<size_t> used_slots;
        for (const auto& match : dorm_iter->second) {
            size_t best_slot = MiniDormLevelRoiTasks.size();
            int best_distance = (std::numeric_limits<int>::max)();
            for (size_t index = 0; index < MiniDormLevelRoiTasks.size(); ++index) {
                if (used_slots.contains(index)) {
                    continue;
                }
                const auto roi_task = asst::Task.get(MiniDormLevelRoiTasks[index]);
                const int distance = squared_distance(center_of(match.rect), center_of(roi_task->roi));
                if (distance < best_distance) {
                    best_slot = index;
                    best_distance = distance;
                }
            }
            if (best_slot == MiniDormLevelRoiTasks.size()) {
                Log.warn("no free mini dorm level ROI", match.rect);
                return false;
            }

            const auto roi_task = asst::Task.get(MiniDormLevelRoiTasks[best_slot]);
            const int level = recognize_level(
                image,
                level_task_name("Dorm"),
                roi_task->roi,
                ViewType::Mini,
                DormMaxLevel);
            if (level == 0) {
                return false;
            }
            used_slots.emplace(best_slot);
            facilities["Dorm"].push_back({ match.rect, level });
        }
    }

    // 右侧四类设施共用白色等级标记，资源任务保存相对于设施匹配矩形的标记区域。
    for (const auto& [facility_name, roi_task_name] : MiniRightLevelRoiTasks) {
        const auto iter = analyzer.get_result().find(std::string(facility_name));
        if (iter == analyzer.get_result().end()) {
            continue;
        }
        const auto roi_task = asst::Task.get(roi_task_name);
        for (const auto& match : iter->second) {
            const int level = recognize_level(
                image,
                roi_task_name,
                match.rect.move(roi_task->rect_move),
                ViewType::Mini,
                StationMaxLevel);
            if (level == 0) {
                return false;
            }
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
        const auto task_name = level_task_name(name);
        const auto level_roi_move = asst::Task.get(task_name)->rect_move;
        for (const auto& match : iter->second) {
            const int level = recognize_level(
                image,
                task_name,
                match.rect.move(level_roi_move),
                ViewType::Normal,
                StationMaxLevel);
            if (level == 0) {
                return false;
            }
            facilities[name].push_back({ match.rect, level });
        }
    }

    const auto dorm_iter = analyzer.get_result().find("Dorm");
    if (dorm_iter == analyzer.get_result().end()) {
        return true;
    }
    const auto task_name = level_task_name("Dorm");
    const auto level_roi_move = asst::Task.get(task_name)->rect_move;
    for (const auto& match : dorm_iter->second) {
        const int level = recognize_level(
            image,
            task_name,
            match.rect.move(level_roi_move),
            ViewType::Normal,
            DormMaxLevel);
        if (level == 0) {
            return false;
        }
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
    return production_count > 0 && production_count <= 9 && counts.dorm <= 4 && counts.control == 1 &&
           counts.reception <= 1 && counts.office <= 1 && counts.processing <= 1 && counts.training <= 1;
}

bool is_complete_layout(const FacilityLayoutCounts& counts)
{
    return counts.production_count() == 9 && counts.mfg != 0 && counts.trade != 0 && counts.power != 0 &&
           counts.dorm == 4 && counts.control == 1 && counts.reception == 1 && counts.office == 1;
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
    const std::array<InputEvent, 9> events = {
        InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 0, .point = pointer0_start },
        InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 1, .point = pointer1_start },
        InputEvent { .type = InputEvent::Type::COMMIT },
        InputEvent { .type = InputEvent::Type::TOUCH_MOVE, .pointerId = 0, .point = pointer0_end },
        InputEvent { .type = InputEvent::Type::TOUCH_MOVE, .pointerId = 1, .point = pointer1_end },
        InputEvent { .type = InputEvent::Type::COMMIT },
        InputEvent { .type = InputEvent::Type::TOUCH_UP, .pointerId = 0 },
        InputEvent { .type = InputEvent::Type::TOUCH_UP, .pointerId = 1 },
        InputEvent { .type = InputEvent::Type::COMMIT },
    };

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
            sleep(300);
            continue;
        }

        std::unordered_map<std::string, std::vector<infrast::FacilityInfo>> facilities;
        const bool levels_recognized =
            analyzer.get_view_type() == ViewType::Mini
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
        if (!levels_recognized || !is_usable_layout(layout_counts)) {
            partial_layout_candidate.reset();
            Log.warn(
                "InfrastInfoTask | inconsistent facility layout, attempt",
                attempt,
                "view",
                static_cast<int>(analyzer.get_view_type()));
            sleep(300);
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
