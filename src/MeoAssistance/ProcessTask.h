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
#include "AsstDef.h"

namespace asst {
    // 流程任务，按照配置文件里的设置的流程运行
    class ProcessTask : public AbstractTask {
    public:
        using AbstractTask::AbstractTask;
        virtual ~ProcessTask() = default;

        virtual bool run() override;

        virtual void set_tasks(const std::vector<std::string>& cur_tasks_name) {
            m_cur_tasks_name = cur_tasks_name;
        }

    protected:
        bool delay_random();
        void exec_click_task(const Rect& matched_rect);
        void exec_swipe_task(ProcessTaskAction action);

        std::vector<std::string> m_cur_tasks_name;
    };
}