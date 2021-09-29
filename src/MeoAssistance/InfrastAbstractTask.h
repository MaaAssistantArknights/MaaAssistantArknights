#pragma once

#include "OcrAbstractTask.h"
#include "AsstDef.h"

namespace asst {
    class InfrastAbstractTask : public OcrAbstractTask
    {
    public:
        InfrastAbstractTask(AsstCallback callback, void* callback_arg);
        virtual ~InfrastAbstractTask() = default;
        virtual bool run() = 0;
        virtual void on_run_fails(int retry_times) override {
            append_task_to_back_to_infrast_home();
        }
    protected:
        virtual bool swipe_to_the_left();   // 滑动到最左侧
        virtual bool click_clear_button();
        virtual bool click_confirm_button();
        virtual bool swipe(bool reverse = false);
        virtual bool swipe_left();          // 往左划（只滑一下）
        virtual bool swipe_right();         // 往右划（只滑一下）
        virtual bool append_task_to_back_to_infrast_home();	// 添加返回主界面的任务

        // 检测干员名
        virtual std::vector<TextArea> detect_operators_name(const cv::Mat& image,
            std::unordered_map<std::string, std::string>& feature_cond,
            std::unordered_set<std::string>& feature_whatever);
        virtual bool enter_station(const std::vector<std::string>& templ_names, int index, double threshold = 0.8);
        virtual bool click_first_operator();

        constexpr static int SwipeExtraDelayDefault = 1000;
        constexpr static int SwipeDurationDefault = 2000;

        double m_cropped_height_ratio = 0.052;	// 图片裁剪出干员名的长条形图片 的高度比例（相比原图）
        double m_cropped_upper_y_ratio = 0.441;	// 图片裁剪出干员名的长条形图片，上半部分干员名的裁剪区域y坐标比例（相比原图）
        double m_cropped_lower_y_ratio = 0.831;	// 图片裁剪出干员名的长条形图片，下半部分干员名的裁剪区域y坐标比例（相比原图）
        Rect m_swipe_begin;									// 边滑动边识别，单次滑动起始点（Rect内随机点）
        Rect m_swipe_end;									// 边滑动边识别，单次滑动结束点（Rect内随机点）
        int m_swipe_duration = SwipeDurationDefault;		// 滑动持续时间，时间越长滑的越慢
        int m_swipe_extra_delay = SwipeExtraDelayDefault;	// 滑动之后额外的等待时间
        int m_swipe_times = 0;								// 滑动了几次，正向滑动增加，反向滑动减少
        bool m_keep_swipe = false;							// keep_swipe函数是否结束的标志位
    };
}
