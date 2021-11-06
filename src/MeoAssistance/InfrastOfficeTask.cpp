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

#include "InfrastOfficeTask.h"

const std::string asst::InfrastOfficeTask::FacilityName = "Office";

bool asst::InfrastOfficeTask::run() {
    json::value task_start_json = json::object{
        { "task_type", "InfrastOfficeTask" },
        { "task_chain", m_task_chain }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    set_facility(FacilityName);
    m_all_available_opers.clear();

    // 办公室只能造这一个
    set_product("HR");

    swipe_to_the_right_of_main_ui();
    enter_facility(FacilityName, 0);
    click_bottomleft_tab();

    constexpr int retry_times = 1;
    for (int i = 0; i <= retry_times; ++i) {
        if (need_exit()) {
            return false;
        }
        swipe_to_the_left_of_operlist();
        opers_detect_with_swipe();
        swipe_to_the_left_of_operlist();

        auto find_iter = std::find_if(m_all_available_opers.begin(), m_all_available_opers.end(),
                                      [&](const InfrastOperSkillInfo& info) -> bool {
                                          return info.selected;
                                      });
        // 如果之前有干员在，那就不换人，直接退出办公室
        if (find_iter != m_all_available_opers.end()) {
            m_all_available_opers.erase(find_iter);
        }
        else {
            optimal_calc();
            bool ret = opers_choose();
            if (!ret) {
                m_all_available_opers.clear();
                continue;
            }
        }
        break;
    }
    click_confirm_button();
    click_return_button();

    return true;
}
