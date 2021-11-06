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
#include "InfrastProductionTask.h"

namespace asst {
    class InfrastReceptionTask : public InfrastProductionTask {
    public:
        using InfrastProductionTask::InfrastProductionTask;
        virtual ~InfrastReceptionTask() = default;
        virtual bool run() override;

        const static std::string FacilityName;

    private:
        bool harvest_clue();
        bool proc_clue();
        bool proc_vacancy();
        bool shift();
        bool swipe_to_the_bottom_of_clue_list_on_the_right();
    };
}
