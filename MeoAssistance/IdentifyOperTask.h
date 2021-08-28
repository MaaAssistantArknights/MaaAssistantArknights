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
		constexpr static int SwipeDuration = 2000;
		constexpr static int SwipeExtraDelay = 1000;

		std::optional<std::unordered_set<OperInfrastInfo>> swipe_and_detect();

		std::vector<TextArea> detect_opers(const cv::Mat& image,
			std::unordered_map<std::string, std::string>& feature_cond,
			std::unordered_set<std::string>& feature_whatever);
		int detect_elite(const cv::Mat& image, const asst::Rect name_rect);

		bool swipe(bool reverse = false);
		bool keep_swipe(bool reverse = false);

		double m_cropped_height_ratio = 0;	// 图片裁剪出干员名的长条形图片 的高度比例（相比原图）
		double m_cropped_upper_y_ratio = 0;	// 图片裁剪出干员名的长条形图片，上半部分干员名的裁剪区域y坐标比例（相比原图）
		double m_cropped_lower_y_ratio = 0;	// 图片裁剪出干员名的长条形图片，下半部分干员名的裁剪区域y坐标比例（相比原图）
		Rect m_swipe_begin;			// 边滑动边识别，单次滑动起始点（Rect内随机点）
		Rect m_swipe_end;			// 边滑动边识别，单次滑动结束点（Rect内随机点）
		bool m_keep_swipe = false;	// keep_swipe函数是否结束的标志位
	};
}
