#pragma once

#include "InfrastAbstractTask.h"

namespace asst {
	class InfrastDormTask : public InfrastAbstractTask
	{
	public:
		using InfrastAbstractTask::InfrastAbstractTask;
		virtual ~InfrastDormTask() = default;

		virtual bool run() override;
		bool set_dorm_begin(int index) {
			if (index < 0 || index >= DormNum) {
				return false;
			}
			m_dorm_begin = index;
			return true;
		}
		bool set_select_with_swipe(bool flag) {
			m_select_with_swipe = flag;
			return true;
		}
	protected:
		struct MoodStatus {
			Rect rect;					// 心情进度条在原图中的位置（总的进度条）
			Rect actual_rect;			// 心情进度条在原图中的位置（白色有效部分）
			int actual_length;			// 心情进度条白色有效部分的长度（像素点数量）
			double process = 0.0;		// 心情进度条剩余百分比
			int time_left_hour = 0.0;	// 剩余工作时间（小时数）
		};
		constexpr static int MaxOperNumInDorm = 5;	// 宿舍最大干员数
		constexpr static int DormNum = 4;			// 宿舍数量

		virtual bool click_confirm_button() override;
		// 进入下一个宿舍
		bool enter_next_dorm();
		// 进入当前宿舍的干员选择界面
		bool enter_operator_selection();
		// 选择干员，返回本次选择了几个干员
		int select_operators(const cv::Mat& image);
		// 选择干员，二次封装，包括截图、点击清除、确定按钮等，返回本次选择了几个干员
		int select_operators(bool need_to_the_left = false);
		// 一边滑动一边选择干员，返回本次选择了几个干员
		int select_operators_with_swipe(bool need_to_the_left = false);
		// 检测正在工作中的干员心情状态
		std::vector<MoodStatus> detect_mood_status_at_work(
			const cv::Mat& image, double process_threshold = 1.0) const;

		int m_dorm_begin = 0;				// 从第几个宿舍开始
		int m_select_with_swipe = false;	// 选择干员是否需要滑动，不滑动即只选择第一页的干员
	};
}
