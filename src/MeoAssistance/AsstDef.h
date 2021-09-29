#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ostream>
#include <functional>

namespace json
{
    class value;
}

namespace asst {
    enum class ProcessTaskAction {
        Invalid = 0,
        BasicClick = 0x100,
        ClickSelf = BasicClick | 1,		// 点击自身位置
        ClickRect = BasicClick | 2,		// 点击指定区域
        ClickRand = BasicClick | 4,		// 点击随机区域
        DoNothing = 0x200,				// 什么都不做
        Stop = 0x400,					// 停止当前Task
        PrintWindow = 0x800,			// 截图
        BasicSwipe = 0x1000,
        SwipeToTheLeft = BasicSwipe | 1,	// 往左划一下
        SwipeToTheRight = BasicSwipe | 2,	// 往右划一下
    };

    enum class AlgorithmType {
        Invaild = -1,
        JustReturn,
        MatchTemplate,
        CompareHist,
        OcrDetect
    };

    struct Point
    {
        Point() = default;
        Point(const Point&) noexcept = default;
        Point(Point&&) noexcept = default;
        Point(int x, int y) : x(x), y(y) {}
        Point& operator=(const Point&) noexcept = default;
        Point& operator=(Point&&) noexcept = default;
        int x = 0;
        int y = 0;
    };

    struct Rect
    {
        Rect() = default;
        Rect(const Rect&) noexcept = default;
        Rect(Rect&&) noexcept = default;
        Rect(int x, int y, int width, int height)
            : x(x), y(y), width(width), height(height) {}
        Rect operator*(double rhs) const
        {
            return { x, y, static_cast<int>(width * rhs), static_cast<int>(height * rhs) };
        }
        Rect center_zoom(double scale, int max_width = INT_MAX, int max_height = INT_MAX) const
        {
            int half_width_scale = static_cast<int>(width * (1 - scale) / 2);
            int half_hight_scale = static_cast<int>(height * (1 - scale) / 2);
            Rect dst(x + half_width_scale, y + half_hight_scale,
                static_cast<int>(width * scale), static_cast<int>(height * scale));
            if (dst.x < 0) { dst.x = 0; }
            if (dst.y < 0) { dst.y = 0; }
            if (dst.width + dst.x >= max_width) { dst.width = max_width - dst.x; }
            if (dst.height + dst.y >= max_height) { dst.height = max_height - dst.y; }
            return dst;
        }
        Rect& operator=(const Rect&) noexcept = default;
        Rect& operator=(Rect&&) noexcept = default;

        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    struct TextArea {
        TextArea() = default;
        TextArea(const TextArea&) = default;
        TextArea(TextArea&&) noexcept = default;
        template<typename ...RectArgs>
        TextArea(std::string text, RectArgs &&... rect_args)
            : text(std::move(text)),
            rect(std::forward<RectArgs>(rect_args)...) {
            static_assert(std::is_constructible<asst::Rect, RectArgs...>::value,
                "Parameter can't be used to construct a asst::Rect");
        }
        operator std::string() const { return text; }
        TextArea& operator=(const TextArea&) = default;
        TextArea& operator=(TextArea&&) noexcept = default;

        std::string text;
        Rect rect;
    };

    struct HandleInfo {
        std::string class_name;
        std::string window_name;
    };

    struct AdbCmd {
        std::string path;
        std::string connect;
        std::string click;
        std::string swipe;
        std::string display;
        std::string display_regex;
        std::string screencap;
        //std::string pullscreen;
        int display_width = 0;
        int display_height = 0;
    };

    struct EmulatorInfo {
        std::string name;
        HandleInfo handle;
        AdbCmd adb;
        std::string path;
    };

    // 任务信息
    struct TaskInfo {
        virtual ~TaskInfo() = default;
        std::string name;								// 任务名
        AlgorithmType algorithm =						// 图像算法类型
            AlgorithmType::Invaild;
        ProcessTaskAction action =						// 要进行的操作
            ProcessTaskAction::Invalid;
        std::vector<std::string> next;					// 下一个可能的任务（列表）
        int exec_times = 0;								// 任务已执行了多少次
        int max_times = INT_MAX;						// 任务最多执行多少次
        std::vector<std::string> exceeded_next;			// 达到最多次数了之后，下一个可能的任务（列表）
        std::vector<std::string> reduce_other_times;	// 执行了该任务后，需要减少别的任务的执行次数。例如执行了吃理智药，则说明上一次点击蓝色开始行动按钮没生效，所以蓝色开始行动要-1
        asst::Rect specific_area;						// 指定区域，目前仅针对ClickRect任务有用，会点这个区域
        int pre_delay = 0;								// 执行该任务前的延时
        int rear_delay = 0;								// 执行该任务后的延时
        int retry_times = INT_MAX;						// 未找到图像时的重试次数
        Rect identify_area;								// 要识别的区域，若为0则全图识别
    };

    // 文字识别任务的信息
    struct OcrTaskInfo : public TaskInfo {
        virtual ~OcrTaskInfo() = default;
        std::vector<std::string> text;					// 文字的容器，匹配到这里面任一个，就算匹配上了
        bool need_match = false;						// 是否需要全匹配，否则搜索到子串就算匹配上了
        std::unordered_map<std::string, std::string>
            replace_map;								// 部分文字容易识别错，字符串强制replace之后，再进行匹配
        bool cache = false;								// 是否使用缓存区域：上次处理该任务时，在一些rect里识别到过text，这次优先在这些rect里识别，省点性能
        std::vector<Rect> cache_area;					// 识别缓存区域
    };

    // 图片匹配任务的信息
    struct MatchTaskInfo : public TaskInfo {
        virtual ~MatchTaskInfo() = default;
        std::string template_filename;					// 匹配模板图片文件名
        double templ_threshold = 0;						// 模板匹配阈值
        double hist_threshold = 0;						// 直方图比较阈值
        bool cache = false;								// 是否使用缓存（直方图），false时就一直用模板匹配。默认为true
    };

    enum class ConnectType {
        Emulator,
        USB,
        Remote
    };

    struct Options {
        ConnectType connect_type;			// 连接类型
        std::string connect_remote_address;	// 连接局域网中的安卓设备的地址，仅在connectType为Remote时生效
        int task_delay = 0;					// 任务间延时：越快操作越快，但会增加CPU消耗
        int control_delay_lower = 0;		// 点击随机延时下限：每次点击操作会进行随机延时
        int control_delay_upper = 0;		// 点击随机延时上限：每次点击操作会进行随机延时
        bool print_window = false;			// 截图功能：开启后每次结算界面会截图到screenshot目录下
        int print_window_delay = 0;			// 截图延时：每次到结算界面，掉落物品不是一次性出来的，有个动画，所以需要等一会再截图
        int ocr_gpu_index = -1;				// OcrLite使用GPU编号，-1(使用CPU)/0(使用GPU0)/1(使用GPU1)/...
        int ocr_thread_number = 0;			// OcrLite线程数量
    };

    struct InfrastOptions {					// 基建的选项
        double dorm_threshold = 0;			// 宿舍心情百分比阈值，心情百分比小于该值的干员会被放进宿舍
    };

    // 干员信息，公开招募相关
    struct OperRecruitInfo {
        std::string name;
        std::string type;
        int level = 0;
        std::string sex;
        std::unordered_set<std::string> tags;
        bool hidden = false;
        std::string name_en;
    };

    // 公开招募的干员组合
    struct OperRecruitCombs {
        std::vector<OperRecruitInfo> opers;
        int max_level = 0;
        int min_level = 0;
        double avg_level = 0;
    };

    // 干员信息，基建相关
    struct OperInfrastInfo {
        std::string name;
        int elite = 0;		// 精英化等级
        int level = 0;		// 等级
        bool operator==(const OperInfrastInfo& rhs) const {
            return name == rhs.name;
        }
    };
    // 基建干员组合
    struct OperInfrastComb {
        std::vector<OperInfrastInfo> comb;
        int efficiency = 0;	// 组合的效率
    };

    constexpr double DoubleDiff = 1e-12;
}

namespace std {
    template<>
    class hash<asst::OperInfrastInfo> {
    public:
        size_t operator()(const asst::OperInfrastInfo& info) const
        {
            return std::hash<std::string>()(info.name);
        }
    };
}
