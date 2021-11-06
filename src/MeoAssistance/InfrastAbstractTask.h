/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "AbstractTask.h"

namespace asst {
    class InfrastAbstractTask : public AbstractTask {
    public:
        using AbstractTask::AbstractTask;
        virtual ~InfrastAbstractTask() = default;

    protected:
        virtual bool enter_facility(const std::string& facility, int index = 0);
        virtual bool enter_oper_list_page(); // 从刚点进基建的界面，到干员列表页

        virtual void swipe_to_the_left_of_operlist(); // 滑动到干员列表的最左侧
        virtual void swipe_to_the_left_of_main_ui();  // 滑动基建的主界面到最左侧
        virtual void swipe_to_the_right_of_main_ui(); // 滑动基建的主界面到最右侧
        virtual void sync_swipe_of_operlist(bool reverse = false);
        virtual void async_swipe_of_operlist(bool reverse = false);
        virtual void await_swipe();

        virtual bool click_bottomleft_tab(); // 点击进入设施后，左下角的tab（我也不知道这玩意该叫啥）
        virtual bool click_clear_button();   // 点击干员选择页面的“清空选择”按钮
        virtual bool click_confirm_button(); // 点击干员选择页面的“确认”按钮

        int m_last_swipe_id = 0;
    };
}
