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
#include "InfrastAbstractTask.h"

namespace asst {
    class InfrastDormTask : public InfrastAbstractTask {
    public:
        using InfrastAbstractTask::InfrastAbstractTask;
        virtual ~InfrastDormTask() = default;
        virtual bool run() override;

        void set_mood_threshold(double mood_threshold) noexcept {
            m_mood_threshold = mood_threshold;
        }

        const static std::string FacilityName;
        const static int MaxNumOfOpers = 5;

    private:
        virtual bool click_confirm_button() override;

        int m_cur_dorm_index = 0;
        int m_max_num_of_dorm = 4;
        double m_mood_threshold = 0;
    };
}
