#pragma once
#include "AbstractTask.h"
#include "AsstDef.h"
#include "AsstInfrastDef.h"

namespace asst
{
    class InfrastAbstractTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        InfrastAbstractTask(AsstCallback callback, void* callback_arg);

        virtual ~InfrastAbstractTask() = default;
        virtual void set_work_mode(infrast::WorkMode work_mode) noexcept;
        virtual void set_mood_threshold(double mood_thres) noexcept;

    protected:
        virtual bool on_run_fails() override;

        virtual bool enter_facility(const std::string& facility, int index = 0);
        virtual bool enter_oper_list_page(); // 从刚点进基建的界面，到干员列表页

        virtual void swipe_to_the_left_of_operlist(); // 滑动到干员列表的最左侧
        virtual void swipe_to_the_left_of_main_ui();  // 滑动基建的主界面到最左侧
        virtual void swipe_to_the_right_of_main_ui(); // 滑动基建的主界面到最右侧
        virtual void swipe_of_operlist(bool reverse = false);
        virtual void async_swipe_of_operlist(bool reverse = false);
        virtual void await_swipe();

        virtual bool click_bottomleft_tab(); // 点击进入设施后，左下角的tab（我也不知道这玩意该叫啥）
        virtual bool click_clear_button();   // 点击干员选择页面的“清空选择”按钮
        virtual bool click_confirm_button(); // 点击干员选择页面的“确认”按钮

        int m_last_swipe_id = 0;
        infrast::WorkMode m_work_mode = infrast::WorkMode::Gentle;
        std::string m_work_mode_name = "Gentle";
        double m_mood_threshold = 0;

        static int m_face_hash_thres;
        static int m_name_hash_thres;
    };
}
