#pragma once
#include "AbstractTask.h"

namespace asst {
    class InfrastAbstractTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~InfrastAbstractTask() = default;
    protected:
        virtual bool enter_facility(const std::string& facility, int index = 0);
        virtual bool enter_oper_list_page();    // 从刚点进基建的界面，到干员列表页

        virtual void sync_swipe_to_the_left_of_operlist();       // 滑动到干员列表的最左侧
        virtual void sync_swipe_of_operlist(bool reverse = false);
        virtual void async_swipe_of_operlist(bool reverse = false);
        virtual void await_swipe();

        virtual void click_clear_button();      // 点击干员选择页面的“清空选择”按钮
        virtual void click_confirm_button();    // 点击干员选择页面的“确认”按钮

        int m_last_swipe_id = 0;
    };
}