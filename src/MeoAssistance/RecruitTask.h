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
    class RecruitTask : public AbstractTask {
    public:
        using AbstractTask::AbstractTask;
        virtual ~RecruitTask() = default;

        virtual bool run() override;
        virtual void set_param(std::vector<int> required_level, bool set_time = true);

    protected:
        std::vector<int> m_required_level;
        bool m_set_time = false;
    };
}
