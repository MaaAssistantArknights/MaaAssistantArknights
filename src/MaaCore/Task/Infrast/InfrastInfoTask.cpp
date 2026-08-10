#include "InfrastInfoTask.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>
#include <unordered_set>

#include "MaaUtils/NoWarningCV.hpp"

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

constexpr Rgb MfgColor { 0xFF, 0xCC, 0x00 };
constexpr Rgb TradeColor { 0x33, 0xCC, 0xFF };
constexpr Rgb PowerColor { 0xCC, 0xFF, 0x66 };
constexpr Rgb DormColor { 0xFF, 0xFF, 0xFF };

// 左侧九个设施及等级点均为 1280x720 最小视图坐标。等级点从左到右
// 分别表示 1、2、3 级；识别时从最高等级向低等级回退。
constexpr std::array<MiniStation, 9> MiniStations = {
    MiniStation { { 124, 320 }, { asst::Point { 195, 308 }, { 200, 308 }, { 205, 308 } } },
    MiniStation { { 277, 320 }, { asst::Point { 347, 308 }, { 352, 308 }, { 358, 308 } } },
    MiniStation { { 425, 320 }, { asst::Point { 498, 308 }, { 504, 308 }, { 509, 308 } } },
    MiniStation { { 51, 393 }, { asst::Point { 119, 382 }, { 125, 382 }, { 129, 382 } } },
    MiniStation { { 199, 393 }, { asst::Point { 271, 382 }, { 277, 382 }, { 282, 382 } } },
    MiniStation { { 350, 393 }, { asst::Point { 423, 382 }, { 428, 382 }, { 433, 382 } } },
    MiniStation { { 124, 455 }, { asst::Point { 195, 459 }, { 200, 459 }, { 205, 459 } } },
    MiniStation { { 277, 455 }, { asst::Point { 347, 459 }, { 352, 459 }, { 358, 459 } } },
    MiniStation { { 425, 455 }, { asst::Point { 498, 459 }, { 504, 459 }, { 509, 459 } } },
};

constexpr std::array<MiniDorm, 4> MiniDorms = {
    MiniDorm { { 698, 303 }, { asst::Point { 657, 307 }, { 663, 307 }, { 669, 307 }, { 674, 307 }, { 680, 307 } } },
    MiniDorm { { 779, 383 }, { asst::Point { 732, 381 }, { 737, 381 }, { 744, 381 }, { 750, 381 }, { 755, 381 } } },
    MiniDorm { { 698, 463 }, { asst::Point { 657, 456 }, { 663, 456 }, { 669, 456 }, { 674, 456 }, { 680, 456 } } },
    MiniDorm { { 779, 543 }, { asst::Point { 732, 533 }, { 737, 533 }, { 744, 533 }, { 750, 533 }, { 755, 533 } } },
};

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

bool is_color_near(const cv::Mat& image, const asst::Point& point, const Rgb& target, bool wide_probe)
{
    const std::array<asst::Point, 5> offsets =
        wide_probe ? std::array<asst::Point, 5> { asst::Point { 0, 0 }, { -5, -10 }, { -5, 10 }, { 5, 10 }, { 5, -10 } }
                   : std::array<asst::Point, 5> { asst::Point { 0, 0 }, { -1, -3 }, { -1, 3 }, { 1, 3 }, { 1, -3 } };
    return std::ranges::any_of(offsets, [&](const asst::Point& offset) {
        return is_color(image, { point.x + offset.x, point.y + offset.y }, target);
    });
}

template <size_t Size>
int recognize_level(const cv::Mat& image, const std::array<asst::Point, Size>& points, const Rgb& color)
{
    for (size_t index = Size; index > 0; --index) {
        if (is_color_near(image, points[index - 1], color, false)) {
            return static_cast<int>(index);
        }
    }
    return 0;
}

std::optional<std::string> recognize_station_type(const cv::Mat& image, const asst::Point& position)
{
    if (is_color_near(image, position, MfgColor, true)) {
        return "Mfg";
    }
    if (is_color_near(image, position, TradeColor, true)) {
        return "Trade";
    }
    if (is_color_near(image, position, PowerColor, true)) {
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
int recognize_normal_level(const cv::Mat& image, const asst::Rect& rect, const Rgb& color, int max_level, bool dorm)
{
    const int x_begin = std::clamp(dorm ? rect.x + rect.width : rect.x + rect.width / 5, 0, image.cols);
    const int x_end = std::clamp(dorm ? rect.x + rect.width + 100 : rect.x + rect.width + 20, 0, image.cols);
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
                if (run_length <= 14) {
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
    std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities)
{
    std::array<std::optional<std::string>, MiniStations.size()> station_types;
    for (size_t index = 0; index < MiniStations.size(); ++index) {
        station_types[index] = recognize_station_type(image, MiniStations[index].position);
        if (!station_types[index]) {
            return false;
        }
    }

    for (const auto& name : { "Mfg", "Trade", "Power" }) {
        const auto iter = analyzer.get_result().find(name);
        if (iter == analyzer.get_result().end()) {
            return false;
        }

        auto& output = facilities[name];
        std::unordered_set<size_t> used;
        for (const auto& match : iter->second) {
            size_t layout_index = MiniStations.size();
            int best_distance = std::numeric_limits<int>::max();
            for (size_t index = 0; index < MiniStations.size(); ++index) {
                if (used.contains(index) || station_types[index] != name) {
                    continue;
                }
                const int distance = squared_distance(center_of(match.rect), MiniStations[index].position);
                if (distance < best_distance) {
                    best_distance = distance;
                    layout_index = index;
                }
            }
            if (layout_index == MiniStations.size()) {
                return false;
            }
            const int level = recognize_level(image, MiniStations[layout_index].level_points, facility_color(name));
            if (level == 0) {
                return false;
            }
            used.emplace(layout_index);
            output.push_back({ match.rect, level });
        }
    }

    const auto dorm_iter = analyzer.get_result().find("Dorm");
    if (dorm_iter == analyzer.get_result().end()) {
        return false;
    }
    std::unordered_set<size_t> used_dorms;
    for (const auto& match : dorm_iter->second) {
        const size_t index = nearest_unused_layout(center_of(match.rect), MiniDorms, used_dorms);
        if (index == MiniDorms.size()) {
            return false;
        }
        const int level = recognize_level(image, MiniDorms[index].level_points, DormColor);
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
    std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities)
{
    for (const auto& name : { "Mfg", "Trade", "Power" }) {
        const auto iter = analyzer.get_result().find(name);
        if (iter == analyzer.get_result().end()) {
            return false;
        }
        for (const auto& match : iter->second) {
            const int level = recognize_normal_level(image, match.rect, facility_color(name), 3, false);
            if (level == 0) {
                return false;
            }
            facilities[name].push_back({ match.rect, level });
        }
    }

    const auto dorm_iter = analyzer.get_result().find("Dorm");
    if (dorm_iter == analyzer.get_result().end()) {
        return false;
    }
    for (const auto& match : dorm_iter->second) {
        const int level = recognize_normal_level(image, match.rect, DormColor, 5, true);
        if (level == 0) {
            return false;
        }
        facilities["Dorm"].push_back({ match.rect, level });
    }
    return true;
}

bool has_complete_layout(const std::unordered_map<std::string, std::vector<asst::infrast::FacilityInfo>>& facilities)
{
    const auto count = [&](std::string_view name) {
        const auto iter = facilities.find(std::string(name));
        return iter == facilities.end() ? size_t { 0 } : iter->second.size();
    };
    const size_t left_count = count("Mfg") + count("Trade") + count("Power");
    return left_count == 9 && count("Mfg") != 0 && count("Trade") != 0 && count("Power") != 0 && count("Dorm") == 4 &&
           count("Control") == 1 && count("Reception") == 1 && count("Office") == 1;
}
} // namespace

bool asst::InfrastInfoTask::try_zoom_out()
{
    // 两指从画面两侧向中心收拢。旧 ADB 和 PlayTools 控制器不支持多指输入，
    // 此时立即返回，后续仍同时使用正常/最小视图模板识别。
    const std::array<InputEvent, 9> events = {
        InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 0, .point = { 980, 180 } },
        InputEvent { .type = InputEvent::Type::TOUCH_DOWN, .pointerId = 1, .point = { 300, 700 } },
        InputEvent { .type = InputEvent::Type::COMMIT },
        InputEvent { .type = InputEvent::Type::TOUCH_MOVE, .pointerId = 0, .point = { 650, 350 } },
        InputEvent { .type = InputEvent::Type::TOUCH_MOVE, .pointerId = 1, .point = { 630, 370 } },
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
    for (int attempt = 1; attempt <= MaxAttempts; ++attempt) {
        if (need_exit()) {
            return false;
        }

        const auto image = ctrler()->get_image();
        InfrastFacilityImageAnalyzer analyzer(image);
        analyzer.set_to_be_analyzed(
            { "Mfg", "Trade", "Power", "Dorm", "Control", "Reception", "Office", "Processing", "Training" });
        if (!analyzer.analyze()) {
            Log.warn("InfrastInfoTask | no facility matched, attempt", attempt);
            sleep(300);
            continue;
        }

        std::unordered_map<std::string, std::vector<infrast::FacilityInfo>> facilities;
        const bool levels_recognized = analyzer.get_view_type() == ViewType::Mini
                                           ? recognize_mini_layout(image, analyzer, facilities)
                                           : recognize_normal_layout(image, analyzer, facilities);

        for (const auto& name : { "Control", "Reception", "Office", "Processing", "Training" }) {
            if (const auto iter = analyzer.get_result().find(name); iter != analyzer.get_result().end()) {
                auto& output = facilities[name];
                std::ranges::transform(iter->second, std::back_inserter(output), [](const MatchRect& match) {
                    return infrast::FacilityInfo { match.rect, 0 };
                });
            }
        }

        if (!levels_recognized || !has_complete_layout(facilities)) {
            Log.warn(
                "InfrastInfoTask | incomplete facility layout, attempt",
                attempt,
                "view",
                static_cast<int>(analyzer.get_view_type()));
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

        int left_level_sum = 0;
        for (const auto& name : { "Mfg", "Trade", "Power", "Dorm" }) {
            for (const auto& facility : m_task_data->facilities[name]) {
                left_level_sum += facility.level;
            }
        }
        // 控制中枢与右侧满设施按其他站等级情况近似。
        m_task_data->total_station_level = left_level_sum + static_cast<int>(std::round(left_level_sum * 17.0 / 47.0));
        return true;
    }

    save_img(utils::path("debug") / utils::path("infrast") / utils::path("facility_layout"), false);
    json::value error = basic_info_with_what("FacilityLayoutRecognitionFailed");
    error["details"]["attempts"] = MaxAttempts;
    callback(AsstMsg::SubTaskError, error);
    Log.error("InfrastInfoTask | facility layout recognition failed after", MaxAttempts, "attempts");
    return false;
}
