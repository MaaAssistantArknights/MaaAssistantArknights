#pragma once
#include "AbstractTask.h"
#include "AsstInfrastDef.h"
#include "AsstTypes.h"

namespace asst
{
    class InfrastAbstractTask : public AbstractTask
    {
    public:
        InfrastAbstractTask(AsstCallback callback, void* callback_arg, std::string task_chain);

        virtual ~InfrastAbstractTask() override = default;
        InfrastAbstractTask& set_mood_threshold(double mood_thres) noexcept;

        virtual json::value basic_info() const override;
        virtual std::string facility_name() const;
        virtual size_t max_num_of_facilities() const noexcept { return 1ULL; }
        virtual size_t max_num_of_opers() const noexcept { return 1ULL; }

        void set_custom_config(infrast::CustomFacilityConfig config) noexcept;
        void clear_custom_config() noexcept;

        static constexpr int OperSelectRetryTimes = 3;
        static constexpr int TaskRetryTimes = 3;

    protected:
        virtual bool on_run_fails() override;

        bool enter_facility(int index = 0);
        bool enter_oper_list_page(); // 从刚点进设施的界面，到干员列表页

        void swipe_to_the_left_of_operlist(int loop_times = 1); // 滑动到干员列表的最左侧
        void swipe_to_the_left_of_main_ui();                    // 滑动基建的主界面到最左侧
        void swipe_to_the_right_of_main_ui();                   // 滑动基建的主界面到最右侧
        void swipe_of_operlist(bool reverse = false);
        void async_swipe_of_operlist(bool reverse = false);
        void await_swipe();
        bool is_use_custom_config();
        bool swipe_and_select_custom_opers(bool is_dorm_order = false);
        bool select_custom_opers();
        void order_opers_selection(const std::vector<std::string>& names);

        virtual bool click_bottom_left_tab(); // 点击进入设施后，左下角的tab（我也不知道这玩意该叫啥）
        virtual bool click_clear_button();         // 点击干员选择页面的“清空选择”按钮
        virtual bool click_sort_by_trust_button(); // 点击干员选择页面的“按信赖值排序”按钮
        virtual bool click_filter_menu_not_stationed_button(); // 点击干员选择页面的筛选菜单按钮的“未进驻”按钮
        virtual bool click_confirm_button();                   // 点击干员选择页面的“确认”按钮

        int m_last_swipe_id = 0;
        const std::string m_work_mode_name =
            "Aggressive"; // 历史遗留问题，之前是分工作模式的，后来发现其他模式都不好用，就全删了只保留了这一个
        double m_mood_threshold = 0;
        mutable std::string m_facility_name_cache;
        int m_cur_facility_index = 0;
        bool m_is_custom = false;
        infrast::CustomFacilityConfig m_custom_config;
        infrast::CustomRoomConfig m_current_room_custom_config;
    };
}
