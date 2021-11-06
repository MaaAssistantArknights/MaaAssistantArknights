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

#include "AbstractConfiger.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace asst {
    struct TaskInfo;

    class TaskData : public AbstractConfiger {
    public:
        virtual ~TaskData() = default;

        bool set_param(const std::string& type, const std::string& param, const std::string& value);

        const std::shared_ptr<TaskInfo> task_ptr(const std::string& name) const noexcept;
        const std::unordered_set<std::string>& get_templ_required() const noexcept;
        std::shared_ptr<TaskInfo> task_ptr(std::string name);
        void clear_exec_times();

    protected:
        virtual bool parse(const json::value& json);

        std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
        std::unordered_set<std::string> m_templ_required;
    };
}
