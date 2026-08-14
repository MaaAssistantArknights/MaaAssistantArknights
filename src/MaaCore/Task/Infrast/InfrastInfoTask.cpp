#include "InfrastInfoTask.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>
#include <unordered_set>

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Utils/Logger.hpp"
#include "Vision/Infrast/InfrastFacilityImageAnalyzer.h"

namespace
{
using ViewType = asst::InfrastFacilityImageAnalyzer::ViewType;

struct Rgb
{
    int red = 0;
    int green = 0;
    int blue = 0;
};

struct MiniStation
{
    asst::Point position;
    std::array<asst::Point, 3> level_points;
};

struct MiniDorm
{
    asst::Point position;
    std::array<asst::Point, 5> level_points;
};

struct NormalLevelGeometry
{
    int station_begin_divisor = 0;
    int dorm_end_offset = 0;
    int station_end_offset = 0;
    int max_segment_width = 0;
};

struct InfrastInfoGeometry
{
    std::array<MiniStation, 9> mini_stations;
    std::array<MiniDorm, 4> mini_dorms;
    std::array<asst::Point, 5> wide_probe_offsets;
    std::array<asst::Point, 5> narrow_probe_offsets;
    NormalLevelGeometry normal_level;
};

constexpr Rgb MfgColor { 0xFF, 0xCC, 0x00 };
constexpr Rgb TradeColor { 0x33, 0xCC, 0xFF };
constexpr Rgb PowerColor { 0xCC, 0xFF, 0x66 };
constexpr Rgb DormColor { 0xFF, 0xFF, 0xFF };

template <size_t Size>
std::optional<std::array<asst::Point, Size>> load_points(std::string_view task_name)
{
    const auto task = asst::Task.get(task_name);
    const auto& params = task->special_params;
    if (params.size() != Size * 2) {
        Log.error("invalid point configuration", task_name, "expected", Size * 2, "actual", params.size());
        return std::nullopt;
    }

    std::array<asst::Point, Size> points;
    for (size_t index = 0; index < Size; ++index) {
        points[index] = { params[index * 2], params[index * 2 + 1] };
    }
    return points;
}

std::optional<InfrastInfoGeometry> load_info_geometry()
{
    const auto station_points = load_points<36>("InfrastInfoMiniStations");
    const auto dorm_points = load_points<24>("InfrastInfoMiniDorms");
    const auto probe_offsets = load_points<10>("InfrastInfoProbeOffsets");
    const auto normal_task = asst::Task.get("InfrastInfoNormalLevelGeometry");
    const auto& normal_params = normal_task->special_params;
    if (!station_points || !dorm_points || !probe_offsets || normal_params.size() != 4 || normal_params[0] <= 0 ||
        normal_params[3] <= 0) {
        if (normal_params.size() != 4) {
            Log.error("invalid InfrastInfoNormalLevelGeometry parameter count", normal_params.size());
        }
        else if (normal_params[0] <= 0 || normal_params[3] <= 0) {
            Log.error("invalid InfrastInfoNormalLevelGeometry values", normal_params[0], normal_params[3]);
        }
        return std::nullopt;
    }

    InfrastInfoGeometry geometry;
    size_t point_index = 0;
    for (auto& station : geometry.mini_stations) {
        station.position = station_points->at(point_index++);
        for (auto& level_point : station.level_points) {
            level_point = station_points->at(point_index++);
        }
    }

    point_index = 0;
    for (auto& dorm : geometry.mini_dorms) {
        dorm.position = dorm_points->at(point_index++);
        for (auto& level_point : dorm.level_points) {
            level_point = dorm_points->at(point_index++);
        }
    }

    std::ranges::copy_n(
        probe_offsets->begin(),
        geometry.wide_probe_offsets.size(),
        geometry.wide_probe_offsets.begin());
    std::ranges::copy_n(
        probe_offsets->begin() + geometry.wide_probe_offsets.size(),
        geometry.narrow_probe_offsets.size(),
        geometry.narrow_probe_offsets.begin());
    geometry.normal_level = {
        .station_begin_divisor = normal_params[0],
        .dorm_end_offset = normal_params[1],
        .station_end_offset = normal_params[2],
        .max_segment_width = normal_params[3],
    };
    return geometry;
}

bool is_color(const cv::Mat& image, const asst::Point& point, const Rgb& target, int tolerance = 42)
{
    if (point.x < 0 || point.y < 0 || point.x >= image.cols || point.y >= image.rows) {
        return false;
    }
    const auto pixel = image.at<cv::Vec3b>(point.y, point.x);
    return std::abs(static_cast<int>(pixel[2]) - target.red) <= tolerance &&
           std::abs(static_cast<int>(pixel[1]) - target.green) <= tolerance &&
           std::abs(static_cast<int>(pixel[0]) - target.blue) <= tolerance;
}

bool is_color_near(
    const cv::Mat& image,
    const asst::Point& point,
    const Rgb& target,
    const std::array<asst::Point, 5>& offsets)
{
    return std::ranges::any_of(offsets, [&](const asst::Point& offset) {
        return is_color(image, { point.x + offset.x, point.y + offset.y }, target);
    });
}

template <size_t Size>
int recognize_level(
    const cv::Mat& image,
    const std::array<asst::Point, Size>& points,
    const Rgb& color,
    const std::array<asst::Point, 5>& offsets)
{
    for (size_t index = Size; index > 0; --index) {
        if (is_color_near(image, points[index - 1], color, offsets)) {
            return static_cast<int>(index);
        }
    }
    return 0;
}

std::optional<std::string>
    recognize_station_type(const cv::Mat& image, const asst::Point& position, const std::array<asst::Point, 5>& offsets)
{
    if (is_color_near(image, position, MfgColor, offsets)) {
        return "Mfg";
    }
    if (is_color_near(image, position, TradeColor, offsets)) {
        return "Trade";
    }
    if (is_color_near(image, position, PowerColor, offsets)) {
        return "Power";
    }
    return std::nullopt;
}

const Rgb& facility_color(std::string_view name)
{
    if (name == "Mfg") {
        return MfgColor;
    }
    if (name == "Trade") {
        return TradeColor;
    }
    return PowerColor;
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

template <typename Layout, size_t Size>
size_t nearest_unused_layout(
    const asst::Point& position,
    const std::array<Layout, Size>& layouts,
    const std::unordered_set<size_t>& used)
{
    size_t result = Size;
    int best_distance = std::numeric_limits<int>::max();
    for (size_t index = 0; index < layouts.size(); ++index) {
        if (used.contains(index)) {
            continue;
        }
        const int distance = squared_distance(position, layouts[index].position);
        if (distance < best_distance) {
            best_distance = distance;
            result = index;
        }
    }
    return result;
}

// 正常视图没有固定的九宫格屏幕坐标，等级点跟随设施卡片移动。
// 从卡片右侧的同色短线中取同一水平线上最多的分段数，避免把左侧类型色条算作等级。
int recognize_normal_level(
    const cv::Mat& image,
    const asst::Rect& rect,
    const Rgb& color,
    int max_level,
    bool dorm,
    const NormalLevelGeometry& geometry)
{
    const int x_begin =
        std::clamp(dorm ? rect.x + rect.width : rect.x + rect.width / geometry.station_begin_divisor, 0, image.cols);
    const int x_end = std::clamp(
        rect.x + rect.width + (dorm ? geometry.dorm_end_offset : geometry.station_end_offset),
        0,
        image.cols);
    const int y_begin = std::clamp(rect.y, 0, image.rows);
    const int y_end = std::clamp(rect.y + rect.height, 0, image.rows);

    int best = 0;
    for (int y = y_begin; y < y_end; ++y) {
        int runs = 0;
        int run_length = 0;
        for (int x = x_begin; x <= x_end; ++x) {
            const bool matched = x < x_end && is_color(image, { x, y }, color);
            if (matched) {
                ++run_length;
            }
            else if (run_length != 0) {
                if (run_length <= geometry.max_segment_width) {
                    ++runs;
                }
                run_length = 0;
            }
        }
        if (runs > 0 && runs <= max_level) {
            best = std::max(best, runs);
        }
    }
    return best;
}

bool recognize_mini_layout(
    const cv::Mat& image,
    const asst::InfrastFacilityImageAnalyzer& analyzer,
    std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities,
    const InfrastInfoGeometry& geometry)
{
    std::array<std::optional<std::string>, 9> station_types;
    for (size_t index = 0; index < geometry.mini_stations.size(); ++index) {
        station_types[index] =
            recognize_station_type(image, geometry.mini_stations[index].position, geometry.wide_probe_offsets);
    }

    for (const auto& name : { "Mfg", "Trade", "Power" }) {
        const size_t layout_count =
            std::ranges::count_if(station_types, [&](const std::optional<std::string>& station_type) {
                return station_type == name;
            });
        const auto iter = analyzer.get_result().find(name);
        if (iter == analyzer.get_result().end()) {
            if (layout_count != 0) {
                return false;
            }
            continue;
        }

        auto& output = facilities[name];
        std::unordered_set<size_t> used;
        for (const auto& match : iter->second) {
            size_t layout_index = geometry.mini_stations.size();
            int best_distance = std::numeric_limits<int>::max();
            for (size_t index = 0; index < geometry.mini_stations.size(); ++index) {
                if (used.contains(index) || station_types[index] != name) {
                    continue;
                }
                const int distance = squared_distance(center_of(match.rect), geometry.mini_stations[index].position);
                if (distance < best_distance) {
                    best_distance = distance;
                    layout_index = index;
                }
            }
            if (layout_index == geometry.mini_stations.size()) {
                return false;
            }
            const int level = recognize_level(
                image,
                geometry.mini_stations[layout_index].level_points,
                facility_color(name),
                geometry.narrow_probe_offsets);
            if (level == 0) {
                return false;
            }
            used.emplace(layout_index);
            output.push_back({ match.rect, level });
        }
        if (used.size() != layout_count) {
            return false;
        }
    }

    const auto dorm_iter = analyzer.get_result().find("Dorm");
    if (dorm_iter == analyzer.get_result().end()) {
        return true;
    }
    std::unordered_set<size_t> used_dorms;
    for (const auto& match : dorm_iter->second) {
        const size_t index = nearest_unused_layout(center_of(match.rect), geometry.mini_dorms, used_dorms);
        if (index == geometry.mini_dorms.size()) {
            return false;
        }
        const int level =
            recognize_level(image, geometry.mini_dorms[index].level_points, DormColor, geometry.narrow_probe_offsets);
        if (level == 0) {
            return false;
        }
        used_dorms.emplace(index);
        facilities["Dorm"].push_back({ match.rect, level });
    }
    return true;
}

bool recognize_normal_layout(
    const cv::Mat& image,
    const asst::InfrastFacilityImageAnalyzer& analyzer,
    std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities,
    const NormalLevelGeometry& geometry)
{
    for (const auto& name : { "Mfg", "Trade", "Power" }) {
        const auto iter = analyzer.get_result().find(name);
        if (iter == analyzer.get_result().end()) {
            continue;
        }
        for (const auto& match : iter->second) {
            const int level = recognize_normal_level(image, match.rect, facility_color(name), 3, false, geometry);
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
    for (const auto& match : dorm_iter->second) {
        const int level = recognize_normal_level(image, match.rect, DormColor, 5, true, geometry);
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
    const auto points = load_points<4>("InfrastInfoZoomOut");
    if (!points) {
        return false;
    }

    // 两指从画面两侧向中心收拢。旧 ADB 和 PlayTools 控制器不支持多指输入，
    // 此时立即返回，后续仍同时使用正常/最小视图模板识别。
    const std::array<InputEvent, 9> events = {
        InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 0, .point = points->at(0) },
        InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 1, .point = points->at(1) },
        InputEvent { .type = InputEvent::Type::COMMIT },
        InputEvent { .type = InputEvent::Type::TOUCH_MOVE, .pointerId = 0, .point = points->at(2) },
        InputEvent { .type = InputEvent::Type::TOUCH_MOVE, .pointerId = 1, .point = points->at(3) },
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

    const auto geometry = load_info_geometry();
    if (!geometry) {
        return false;
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
                ? recognize_mini_layout(image, analyzer, facilities, *geometry)
                : recognize_normal_layout(image, analyzer, facilities, geometry->normal_level);

        for (const auto& name : { "Control", "Reception", "Office", "Processing", "Training" }) {
            if (const auto iter = analyzer.get_result().find(name); iter != analyzer.get_result().end()) {
                auto& output = facilities[name];
                std::ranges::transform(iter->second, std::back_inserter(output), [](const MatchRect& match) {
                    return infrast::FacilityInfo { match.rect, 0 };
                });
            }
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
        for (const auto& name : { "Mfg", "Trade", "Power", "Dorm" }) {
            for (const auto& facility : m_task_data->facilities[name]) {
                recognized_level_sum += facility.level;
            }
        }
        constexpr int StationMaxLevel = 3;
        constexpr int DormMaxLevel = 5;
        // 控制中枢等级由宿舍数量推导；其他右侧设施按已识别设施的平均升级比例估算。
        const int recognized_max_level =
            static_cast<int>(layout_counts.production_count() * StationMaxLevel + layout_counts.dorm * DormMaxLevel);
        const int unrecognized_right_max_level = static_cast<int>(
            (layout_counts.reception + layout_counts.office + layout_counts.processing + layout_counts.training) *
            StationMaxLevel);
        const int estimated_right_level = static_cast<int>(std::round(
            recognized_level_sum * static_cast<double>(unrecognized_right_max_level) / recognized_max_level));
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
