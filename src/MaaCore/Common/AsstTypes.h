#pragma once

#include <climits>
#include <cmath>
#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Utils/StringMisc.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace json
{
    class value;
}

namespace asst
{
    constexpr double DoubleDiff = 1e-12;

    static constexpr int WindowWidthDefault = 1280;
    static constexpr int WindowHeightDefault = 720;

    static constexpr double TemplThresholdDefault = 0.8;

    enum class StaticOptionKey
    {
        Invalid = 0,
    };

    enum class InstanceOptionKey
    {
        Invalid = 0,
        /* Deprecated */         // MinitouchEnabled = 1,
        TouchMode = 2,           // 触控模式设置， "minitouch" | "maatouch" | "adb"
        DeploymentWithPause = 3, // 自动战斗、肉鸽、保全 是否使用 暂停下干员， "0" | "1"
    };

    struct Point
    {
        Point() = default;
        ~Point() = default;
        Point(const Point&) noexcept = default;
        Point(Point&&) noexcept = default;
        constexpr Point(int x, int y) : x(x), y(y) {}
        Point& operator=(const Point&) noexcept = default;
        Point& operator=(Point&&) noexcept = default;
        Point operator-() const noexcept { return { -x, -y }; }
        bool operator==(const Point& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
        std::string to_string() const { return "(" + std::to_string(x) + ", " + std::to_string(y) + ")"; }
        explicit operator std::string() const { return to_string(); }
        static constexpr Point right() { return { 1, 0 }; }
        static constexpr Point down() { return { 0, 1 }; }
        static constexpr Point left() { return { -1, 0 }; }
        static constexpr Point up() { return { 0, -1 }; }
        static constexpr Point zero() { return { 0, 0 }; }
        bool empty() const noexcept { return x == 0 && y == 0; }

        int x = 0;
        int y = 0;

#define DEFINE_ASST_POINT_BINARY_OP_AND_ARG_ASSIGN(Op)                    \
    friend Point operator Op(const Point& lhs, const Point& rhs) noexcept \
    {                                                                     \
        return { lhs.x Op rhs.x, lhs.y Op rhs.y };                        \
    }                                                                     \
    friend Point& operator Op##=(Point& val, const Point& opd) noexcept   \
    {                                                                     \
        val.x Op## = opd.x;                                               \
        val.y Op## = opd.y;                                               \
        return val;                                                       \
    }

        DEFINE_ASST_POINT_BINARY_OP_AND_ARG_ASSIGN(+)
        DEFINE_ASST_POINT_BINARY_OP_AND_ARG_ASSIGN(-)
        DEFINE_ASST_POINT_BINARY_OP_AND_ARG_ASSIGN(*)

#undef DEFINE_ASST_POINT_BINARY_OP_AND_ARG_ASSIGN

        friend Point operator*(int scalar, const Point& value) noexcept
        {
            return { value.x * scalar, value.y * scalar };
        }
        friend Point operator*(const Point& value, int scalar) noexcept
        {
            return { value.x * scalar, value.y * scalar };
        }
        static int dot(const Point& lhs, const Point& rhs) noexcept { return (lhs.x * rhs.x) + (lhs.y * rhs.y); }
        static double distance(const Point& lhs, const Point& rhs) noexcept
        {
            return std::sqrt(std::pow(rhs.x - lhs.x, 2) + std::pow(rhs.y - lhs.y, 2));
        }
        double length() const noexcept { return std::sqrt(static_cast<double>(dot(*this, *this))); }
    };

    struct Rect
    {
        Rect() = default;
        ~Rect() = default;
        Rect(const Rect&) noexcept = default;
        Rect(Rect&&) noexcept = default;
        Rect(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
        Rect operator*(double rhs) const
        {
            return { x, y, static_cast<int>(width * rhs), static_cast<int>(height * rhs) };
        }
        int area() const noexcept { return width * height; }
        Rect center_zoom(double scale, int max_width = INT_MAX, int max_height = INT_MAX) const
        {
            const int half_width_scale = static_cast<int>(width * (1 - scale) / 2);
            const int half_height_scale = static_cast<int>(height * (1 - scale) / 2);
            Rect dst(x + half_width_scale, y + half_height_scale, static_cast<int>(width * scale),
                     static_cast<int>(height * scale));
            if (dst.x < 0) {
                dst.x = 0;
            }
            if (dst.y < 0) {
                dst.y = 0;
            }
            if (dst.width + dst.x >= max_width) {
                dst.width = max_width - dst.x;
            }
            if (dst.height + dst.y >= max_height) {
                dst.height = max_height - dst.y;
            }
            return dst;
        }
        Rect& operator=(const Rect&) noexcept = default;
        Rect& operator=(Rect&&) noexcept = default;
        bool operator==(const Rect& rhs) const noexcept
        {
            return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
        }
        bool empty() const noexcept { return width == 0 || height == 0; }
        bool include(const Rect& rhs) const noexcept
        {
            return x <= rhs.x && y <= rhs.y && (x + width) >= (rhs.x + rhs.width) &&
                   (y + height) >= (rhs.y + rhs.height);
        }
        bool include(const Point& rhs) const noexcept
        {
            return x <= rhs.x && y <= rhs.y && (x + width) >= rhs.x && (y + height) >= rhs.y;
        }
        std::string to_string() const
        {
            return "[ " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(width) + ", " +
                   std::to_string(height) + " ]";
        }
        explicit operator std::string() const { return to_string(); }
        Rect move(Rect move) const { return { x + move.x, y + move.y, move.width, move.height }; }

        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    template <typename To, typename From>
    inline constexpr To make_rect(const From& rect)
    {
        return To { rect.x, rect.y, rect.width, rect.height };
    }

    struct TextRect
    {
        TextRect() = default;
        ~TextRect() = default;
        TextRect(const TextRect&) = default;
        TextRect(TextRect&&) noexcept = default;
        TextRect(double score, const Rect& rect, const std::string& text) : score(score), rect(rect), text(text) {}

        TextRect& operator=(const TextRect&) = default;
        TextRect& operator=(TextRect&&) noexcept = default;
        bool operator==(const TextRect& rhs) const noexcept { return text == rhs.text && rect == rhs.rect; }
        bool operator==(const std::string& rhs) const noexcept { return text == rhs; }
        explicit operator Rect() const noexcept { return rect; }
        std::string to_string() const
        {
            return "{ " + text + ": " + rect.to_string() + ", score: " + std::to_string(score) + " }";
        }
        explicit operator std::string() const { return to_string(); }

        double score = 0.0;
        Rect rect;
        std::string text;
    };
    using TextRectProc = std::function<bool(TextRect&)>;

    struct MatchRect
    {
        MatchRect() = default;
        ~MatchRect() = default;
        MatchRect(const MatchRect&) = default;
        MatchRect(MatchRect&&) noexcept = default;
        MatchRect(double score, const Rect& rect) : score(score), rect(rect) {}

        explicit operator Rect() const noexcept { return rect; }
        MatchRect& operator=(const MatchRect&) = default;
        MatchRect& operator=(MatchRect&&) noexcept = default;
        std::string to_string() const
        {
            return "{ rect: " + rect.to_string() + ", score: " + std::to_string(score) + " }";
        }
        explicit operator std::string() const { return to_string(); }

        double score = 0.0;
        Rect rect;
    };
} // namespace asst

namespace std
{
    template <>
    struct hash<asst::Point>
    {
        size_t operator()(const asst::Point& point) const noexcept
        {
            return std::hash<int>()(point.x) ^ std::hash<int>()(point.y);
        }
    };

    template <>
    struct hash<asst::Rect>
    {
        size_t operator()(const asst::Rect& rect) const noexcept
        {
            return std::hash<int>()(rect.x) ^ std::hash<int>()(rect.y) ^ std::hash<int>()(rect.width) ^
                   std::hash<int>()(rect.height);
        }
    };
    template <>
    struct hash<asst::TextRect>
    {
        size_t operator()(const asst::TextRect& tr) const noexcept
        {
            return std::hash<std::string>()(tr.text) ^ std::hash<asst::Rect>()(tr.rect);
        }
    };
}

namespace asst
{
    enum class AlgorithmType
    {
        Invalid = -1,
        JustReturn,
        MatchTemplate,
        OcrDetect,
        Hash
    };

    inline AlgorithmType get_algorithm_type(std::string algorithm_str)
    {
        utils::tolowers(algorithm_str);
        static const std::unordered_map<std::string_view, AlgorithmType> algorithm_map = {
            { "matchtemplate", AlgorithmType::MatchTemplate },
            { "justreturn", AlgorithmType::JustReturn },
            { "ocrdetect", AlgorithmType::OcrDetect },
            { "hash", AlgorithmType::Hash },
        };
        if (algorithm_map.contains(algorithm_str)) {
            return algorithm_map.at(algorithm_str);
        }
        return AlgorithmType::Invalid;
    }

    inline std::string enum_to_string(AlgorithmType algo)
    {
        static const std::unordered_map<AlgorithmType, std::string> algorithm_map = {
            { AlgorithmType::Invalid, "Invalid" },
            { AlgorithmType::JustReturn, "JustReturn" },
            { AlgorithmType::MatchTemplate, "MatchTemplate" },
            { AlgorithmType::OcrDetect, "OcrDetect" },
            { AlgorithmType::Hash, "Hash" },
        };
        if (auto it = algorithm_map.find(algo); it != algorithm_map.end()) {
            return it->second;
        }
        return "Invalid";
    }

    enum class ProcessTaskAction
    {
        Invalid = 0,
        BasicClick = 0x100,
        ClickSelf = BasicClick | 1, // 点击自身位置
        ClickRect = BasicClick | 2, // 点击指定区域
        ClickRand = BasicClick | 4, // 点击随机区域
        DoNothing = 0x200,          // 什么都不做
        Stop = 0x400,               // 停止当前Task
        Swipe = 0x1000,             // 滑动
    };

    inline ProcessTaskAction get_action_type(std::string action_str)
    {
        utils::tolowers(action_str);
        static const std::unordered_map<std::string, ProcessTaskAction> action_map = {
            { "clickself", ProcessTaskAction::ClickSelf }, { "clickrand", ProcessTaskAction::ClickRand },
            { "", ProcessTaskAction::DoNothing },          { "donothing", ProcessTaskAction::DoNothing },
            { "stop", ProcessTaskAction::Stop },           { "clickrect", ProcessTaskAction::ClickRect },
            { "swipe", ProcessTaskAction::Swipe },
        };
        if (auto it = action_map.find(action_str); it != action_map.end()) {
            return it->second;
        }
        return ProcessTaskAction::Invalid;
    }

    inline std::string enum_to_string(ProcessTaskAction action)
    {
        static const std::unordered_map<ProcessTaskAction, std::string> action_map = {
            { ProcessTaskAction::Invalid, "Invalid" },     { ProcessTaskAction::BasicClick, "BasicClick" },
            { ProcessTaskAction::ClickSelf, "ClickSelf" }, { ProcessTaskAction::ClickRect, "ClickRect" },
            { ProcessTaskAction::ClickRand, "ClickRand" }, { ProcessTaskAction::DoNothing, "DoNothing" },
            { ProcessTaskAction::Stop, "Stop" },           { ProcessTaskAction::Swipe, "Swipe" },
        };
        if (auto it = action_map.find(action); it != action_map.end()) {
            return it->second;
        }
        return "Invalid";
    }
} // namespace asst

namespace asst
{
    // 任务信息
    struct TaskInfo
    {
        TaskInfo() = default;
        virtual ~TaskInfo() = default;
        TaskInfo(const TaskInfo&) = default;
        TaskInfo(TaskInfo&&) noexcept = default;
        TaskInfo& operator=(const TaskInfo&) = default;
        TaskInfo& operator=(TaskInfo&&) noexcept = default;
        std::string name;         // 任务名
        AlgorithmType algorithm = // 图像算法类型
            AlgorithmType::Invalid;
        ProcessTaskAction action = // 要进行的操作
            ProcessTaskAction::Invalid;
        std::vector<std::string> sub;           // 子任务（列表）
        bool sub_error_ignored = false;         // 子任务如果失败了，是否继续执行剩下的任务
        std::vector<std::string> next;          // 下一个可能的任务（列表）
        int max_times = INT_MAX;                // 任务最多执行多少次
        std::vector<std::string> exceeded_next; // 达到最多次数了之后，下一个可能的任务（列表）
        std::vector<std::string> on_error_next; // 任务出错之后要去执行什么
        std::vector<std::string> reduce_other_times; // 执行了该任务后，需要减少别的任务的执行次数。
                                                     // 例如执行了吃理智药，则说明上一次点击蓝色开始行动按钮没生效，
                                                     // 所以蓝色开始行动要-1
        Rect specific_rect;        // 指定区域，目前仅针对ClickRect任务有用，会点这个区域
        int pre_delay = 0;         // 执行该任务前的延时
        int post_delay = 0;        // 执行该任务后的延时
        int retry_times = INT_MAX; // 未找到图像时的重试次数
        Rect roi;                  // 要识别的区域，若为0则全图识别
        Rect rect_move;     // 识别结果移动：有些结果识别到的，和要点击的不是同一个位置。
                            // 即识别到了res，点击res + result_move的位置
        bool cache = false; // 是否使用缓存区域
        std::vector<int> special_params; // 某些任务会用到的特殊参数
    };

    // 文字识别任务的信息
    struct OcrTaskInfo : public TaskInfo
    {
        OcrTaskInfo() = default;
        virtual ~OcrTaskInfo() override = default;
        OcrTaskInfo(const OcrTaskInfo&) = default;
        OcrTaskInfo(OcrTaskInfo&&) noexcept = default;
        OcrTaskInfo& operator=(const OcrTaskInfo&) = default;
        OcrTaskInfo& operator=(OcrTaskInfo&&) noexcept = default;
        std::vector<std::string> text; // 文字的容器，匹配到这里面任一个，就算匹配上了
        bool full_match = false;       // 是否需要全匹配，否则搜索到子串就算匹配上了
        bool is_ascii = false;         // 是否启用字符数字模型
        bool without_det = false;      // 是否不使用检测模型
        std::unordered_map<std::string, std::string>
            replace_map; // 部分文字容易识别错，字符串强制replace之后，再进行匹配
    };

    // 图片匹配任务的信息
    struct MatchTaskInfo : public TaskInfo
    {
        MatchTaskInfo() = default;
        virtual ~MatchTaskInfo() override = default;
        MatchTaskInfo(const MatchTaskInfo&) = default;
        MatchTaskInfo(MatchTaskInfo&&) noexcept = default;
        MatchTaskInfo& operator=(const MatchTaskInfo&) = default;
        MatchTaskInfo& operator=(MatchTaskInfo&&) noexcept = default;
        std::string templ_name;         // 匹配模板图片文件名
        double templ_threshold = 0;     // 模板匹配阈值
        std::pair<int, int> mask_range; // 掩码的二值化范围
    };

    // hash 计算任务的信息
    struct HashTaskInfo : public TaskInfo
    {
        HashTaskInfo() = default;
        virtual ~HashTaskInfo() override = default;
        HashTaskInfo(const HashTaskInfo&) = default;
        HashTaskInfo(HashTaskInfo&&) noexcept = default;
        HashTaskInfo& operator=(const HashTaskInfo&) = default;
        HashTaskInfo& operator=(HashTaskInfo&&) noexcept = default;
        std::vector<std::string> hashes; // 需要多个哈希值
        int dist_threshold = 0;          // 汉明距离阈值
        std::pair<int, int> mask_range;  // 掩码的二值化范围
        bool bound = false;              // 是否裁剪周围黑边
    };

    inline static const std::string UploadDataSource = "MeoAssistant";
    inline static constexpr std::string_view RoguelikePhantomThemeName = "Phantom";
    inline static constexpr std::string_view RoguelikeMizukiThemeName = "Mizuki";
} // namespace asst
