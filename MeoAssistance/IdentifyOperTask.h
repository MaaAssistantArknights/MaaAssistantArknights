#pragma once

#include "OcrAbstractTask.h"

namespace asst {
	// 识别干员任务，会将识别到的信息写入文件中
	class IdentifyOperTask : public OcrAbstractTask
	{
	public:
		IdentifyOperTask(AsstCallback callback, void* callback_arg);
		virtual ~IdentifyOperTask() = default;

		virtual bool run() override;

	protected:
		constexpr static int SwipeExtraDelayDefault = 1000;
		constexpr static int SwipeDurationDefault = 2000;

		// 一边滑动一边识别
		virtual std::optional<std::unordered_map<std::string, OperInfrastInfo>> swipe_and_detect();

		// 检测干员名
		std::vector<TextArea> detect_opers(const cv::Mat& image,
			std::unordered_map<std::string, std::string>& feature_cond,
			std::unordered_set<std::string>& feature_whatever);
		// 检测干员精英化等级
		int detect_elite(const cv::Mat& image, const asst::Rect name_rect);

		bool swipe(bool reverse = false);
		bool keep_swipe(bool reverse = false);

		double m_cropped_height_ratio = 0;	// 图片裁剪出干员名的长条形图片 的高度比例（相比原图）
		double m_cropped_upper_y_ratio = 0;	// 图片裁剪出干员名的长条形图片，上半部分干员名的裁剪区域y坐标比例（相比原图）
		double m_cropped_lower_y_ratio = 0;	// 图片裁剪出干员名的长条形图片，下半部分干员名的裁剪区域y坐标比例（相比原图）
		Rect m_swipe_begin;									// 边滑动边识别，单次滑动起始点（Rect内随机点）
		Rect m_swipe_end;									// 边滑动边识别，单次滑动结束点（Rect内随机点）
		int m_swipe_duration = SwipeDurationDefault;		// 滑动持续时间，时间越长滑的越慢
		int m_swipe_extra_delay = SwipeExtraDelayDefault;	// 滑动之后额外的等待时间
		int m_swipe_times = 0;								// 滑动了几次，正向滑动增加，反向滑动减少
		bool m_keep_swipe = false;							// keep_swipe函数是否结束的标志位
	};
}
