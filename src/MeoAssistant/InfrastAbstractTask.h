#pragma once
#include "AbstractTask.h"
#include "AsstTypes.h"
#include "AsstInfrastDef.h"

namespace asst
{
    class InfrastAbstractTask : public AbstractTask
    {
    public:
        InfrastAbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain);

        virtual ~InfrastAbstractTask() override = default;
        InfrastAbstractTask& set_work_mode(infrast::WorkMode work_mode) noexcept;
        InfrastAbstractTask& set_mood_threshold(double mood_thres) noexcept;

        virtual json::value basic_info() const override;
        virtual std::string facility_name() const;
        virtual size_t max_num_of_facilities() const noexcept { return 1ULL; }
        virtual size_t max_num_of_opers() const noexcept { return 1ULL; }

        constexpr static int OperSelectRetryTimes = 3;
        constexpr static int TaskRetryTimes = 3;
    protected:
        virtual bool on_run_fails() override;

        bool enter_facility(int index = 0);
        bool enter_oper_list_page(); // 从刚点进设施的界面，到干员列表页

        void swipe_to_the_left_of_operlist(int loop_times = 1); // 滑动到干员列表的最左侧
        void swipe_to_the_left_of_main_ui();  // 滑动基建的主界面到最左侧
        void swipe_to_the_right_of_main_ui(); // 滑动基建的主界面到最右侧
        void swipe_of_operlist(bool reverse = false);
        void async_swipe_of_operlist(bool reverse = false);
        void await_swipe();

        virtual bool click_bottom_left_tab(); // 点击进入设施后，左下角的tab（我也不知道这玩意该叫啥）
        virtual bool click_clear_button();   // 点击干员选择页面的“清空选择”按钮
        virtual bool click_confirm_button(); // 点击干员选择页面的“确认”按钮

        int m_last_swipe_id = 0;
        infrast::WorkMode m_work_mode = infrast::WorkMode::Aggressive;
        std::string m_work_mode_name = "Aggressive";
        double m_mood_threshold = 0;
        mutable std::string m_facility_name_cache;
        int m_cur_facility_index = 0;
    };
}
