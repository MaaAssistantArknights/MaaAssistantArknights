#pragma once

#include "OcrAbstractTask.h"

namespace asst {
	class InfrastDormTask : public OcrAbstractTask
	{
	public:
		InfrastDormTask(AsstCallback callback, void* callback_arg);
		virtual ~InfrastDormTask() = default;

		virtual bool run() override;

	protected:
		struct MoodStatus {
			Rect rect;					// 心情进度条在原图中的位置（总的进度条）
			Rect actual_rect;			// 心情进度条在原图中的位置（白色有效部分）
			double process = 0.0;		// 心情进度条剩余百分比
			int time_left_hour = 0.0;	// 剩余工作时间（小时数）
		};
		constexpr static int MaxOperNumInDorm = 5;	// 宿舍最大干员数

		// 进入宿舍，index为从上到下的编号
		bool enter_dorm(int index = 0);

		// 进入下一个宿舍
		bool enter_next_dorm();

		// 进入当前宿舍的干员选择界面
		bool enter_operator_selection();

		// 清空并选择原有的“休息中”干员（相当于把“空闲中”的干员去掉）和“注意力涣散”的干员
		bool clear_and_select_the_resting();

		// 检测正在工作中的干员心情状态
		std::vector<MoodStatus> detect_mood_status_at_work(const cv::Mat& image);
	};
}
