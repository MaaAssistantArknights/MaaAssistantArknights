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

	enum class HandleType
	{
		Window = 1,
		View = 2,
		Control = 4
	};

	static std::ostream& operator<<(std::ostream& os, const HandleType& type)
	{
		static std::unordered_map<HandleType, std::string> _type_name = {
			{HandleType::Window, "Window"},
			{HandleType::View, "View"},
			{HandleType::Control, "Control"}
		};
		return os << _type_name.at(type);
	}

	enum class MatchTaskType {
		Invalid = 0,
		BasicClick = 0x100,
		ClickSelf = BasicClick | 1,		// 点击模板自身位置
		ClickRect = BasicClick | 2,		// 点击指定区域
		ClickRand = BasicClick | 4,		// 点击随机区域
		DoNothing = 0x200,				// 什么都不做
		Stop =  0x400,					// 停止工作线程
		PrintWindow =  0x800,			// 截图
		Ocr =  0x1000					// 文字识别任务
	};

	enum class AlgorithmType {
		JustReturn,
		MatchTemplate,
		CompareHist,
		OcrDetect
	};

	struct Point
	{
		Point() = default;
		Point(int x, int y) : x(x), y(y) {}
		int x = 0;
		int y = 0;
	};

	struct Rect
	{
		Rect() = default;
		Rect(int x, int y, int width, int height)
			: x(x), y(y), width(width), height(height) {}
		Rect operator*(double rhs) const
		{
			return { x, y, static_cast<int>(width * rhs), static_cast<int>(height * rhs) };
		}
		Rect center_zoom(double scale) const
		{
			int half_width_scale = static_cast<int>(width * (1 - scale) / 2);
			int half_hight_scale = static_cast<int>(height * (1 - scale) / 2);
			return { x + half_width_scale, y + half_hight_scale, width - half_width_scale,  height - half_hight_scale };
		}
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};

	struct TextArea {
		TextArea() = default;
		TextArea(const TextArea&) = default;
		TextArea(TextArea&&) = default;
		template<typename ...RectArgs>
		TextArea(std::string text, RectArgs &&... rect_args)
			: text(std::move(text)),
			rect(std::forward<RectArgs>(rect_args)...) {
			static_assert(std::is_constructible<asst::Rect, RectArgs...>::value,
				"Parameter can't be used to construct a asst::Rect");
		}
		operator std::string() const { return text; }
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
		std::string display;
		std::string display_regex;
		int display_width = 0;
		int display_height = 0;
	};

	struct EmulatorInfo {
		std::string name;
		std::vector<HandleInfo> window;
		std::vector<HandleInfo> view;
		std::vector<HandleInfo> control;
		bool is_adb = false;
		AdbCmd adb;
		int width = 0;
		int height = 0;
		int x_offset = 0;
		int y_offset = 0;
		int right_offset = 0;
		int bottom_offset = 0;
	};


	struct TaskInfo {
		virtual ~TaskInfo() = 0;
		std::string name;								// 任务名
		MatchTaskType type = MatchTaskType::Invalid;	// 任务类型
		std::vector<std::string> next;					// 下一个可能的任务（列表）
		int exec_times = 0;								// 任务已执行了多少次
		int max_times = INT_MAX;						// 任务最多执行多少次
		std::vector<std::string> exceeded_next;			// 达到最多次数了之后，下一个可能的任务（列表）
		std::vector<std::string> reduce_other_times;	// 执行了该任务后，需要减少别的任务的执行次数。例如执行了吃理智药，则说明上一次点击蓝色开始行动按钮没生效，所以蓝色开始行动要-1
		asst::Rect specific_area;						// 指定区域，目前仅针对ClickRect任务有用，会点这个区域
		int pre_delay = 0;								// 执行该任务前的延时
		int rear_delay = 0;								// 执行该任务后的延时
		int retry_times = INT_MAX;						// 未找到图像时的重试次数
	};

	struct OcrTaskInfo : public TaskInfo {
		virtual ~OcrTaskInfo() = default;
		std::vector<std::string> text;					// 文字的容器，匹配到这里面任一个，就算匹配上了
		bool need_match = false;						// 是否需要全匹配，否则搜索到子串就算匹配上了
		std::unordered_map<std::string, std::string>
			replace_map;								// 部分文字容易识别错，字符串强制replace之后，再进行匹配
	};

	struct MatchTaskInfo : public TaskInfo {
		virtual ~MatchTaskInfo() = default;
		std::string template_filename;					// 匹配模板图片文件名
		double templ_threshold = 0;						// 模板匹配阈值
		double hist_threshold = 0;						// 直方图比较阈值
	};

	struct Options {
		bool identify_cache = false;		// 图像识别缓存功能：开启后可以大幅降低CPU消耗，但需要保证要识别的按钮每次的位置不会改变
		int task_identify_delay = 0;		// 识别任务间延时：越快操作越快，但会增加CPU消耗
		int task_control_delay = 0;			// 点击任务间延迟：越快点的越快，但是太快了句柄可能不响应
		int control_delay_lower = 0;		// 点击随机延时下限：每次点击操作会进行随机延时
		int control_delay_upper = 0;		// 点击随机延时上限：每次点击操作会进行随机延时
		bool print_window = false;			// 截图功能：开启后每次结算界面会截图到screenshot目录下
		int print_window_delay = 0;			// 截图延时：每次到结算界面，掉落物品不是一次性出来的，有个动画，所以需要等一会再截图
		int print_window_crop_offset = 0;	// 截图额外裁剪：再额外把边框裁减掉一圈，不然企鹅物流有可能识别不出来
		int ocr_gpu_index = -1;				// OcrLite使用GPU编号，-1(使用CPU)/0(使用GPU0)/1(使用GPU1)/...
		int ocr_thread_number = 0;			// OcrLite线程数量
	};

	// 干员信息
	struct OperInfo {
		std::string name;
		std::string type;
		int level = 0;
		std::string sex;
		std::unordered_set<std::string> tags;
		bool hidden = false;
		std::string name_en;
	};

	// 干员组合
	struct OperCombs {
		std::vector<OperInfo> opers;
		int max_level = 0;
		int min_level = 0;
		double avg_level = 0;
	};

	constexpr double DoubleDiff = 1e-12;
}