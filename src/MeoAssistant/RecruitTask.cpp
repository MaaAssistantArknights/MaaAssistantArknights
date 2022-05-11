#include "RecruitTask.h"

#include "AutoRecruitTask.h"
#include "RecruitCalcTask.h"
#include "ProcessTask.h"

asst::RecruitTask::RecruitTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_recruit_begin_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType)),
    m_auto_recruit_task_ptr(std::make_shared<AutoRecruitTask>(callback, callback_arg, TaskType)),
    m_recruit_only_calc_task_ptr(std::make_shared<RecruitCalcTask>(callback, callback_arg, TaskType))
{
    m_recruit_begin_task_ptr->set_tasks({ "RecruitBegin" });
    m_recruit_only_calc_task_ptr->set_enable(false);

    m_subtasks.emplace_back(m_recruit_begin_task_ptr);
    m_subtasks.emplace_back(m_auto_recruit_task_ptr);
    m_subtasks.emplace_back(m_recruit_only_calc_task_ptr);
}

bool asst::RecruitTask::set_params(const json::value& params)
{
    if (!params.contains("select") || !params.at("select").is_array()
        || !params.contains("confirm") || !params.at("confirm").is_array()) {
        return false;
    }

    std::vector<int> select;
    for (const auto& select_num_json : params.at("select").as_array()) {
        if (!select_num_json.is_number()) {
            return false;
        }
        select.emplace_back(select_num_json.as_integer());
    }

    std::vector<int> confirm;
    for (const auto& confirm_num_json : params.at("confirm").as_array()) {
        if (!confirm_num_json.is_number()) {
            return false;
        }
        confirm.emplace_back(confirm_num_json.as_integer());
    }

    bool refresh = params.get("refresh", false);
    bool set_time = params.get("set_time", true);
    int times = params.get("times", 0);
    bool expedite = params.get("expedite", false);
    [[maybe_unused]] int expedite_times = params.get("expedite_times", 0);

    if (times <= 0 || confirm.empty()) {   // 仅识别的情况
        m_recruit_begin_task_ptr->set_enable(false);
        m_recruit_only_calc_task_ptr->set_enable(true);
        m_auto_recruit_task_ptr->set_enable(false);

        m_recruit_only_calc_task_ptr->set_param(std::move(select), set_time);
    }
    else {
        m_recruit_begin_task_ptr->set_enable(true);
        m_recruit_only_calc_task_ptr->set_enable(false);
        m_auto_recruit_task_ptr->set_enable(true);

        m_auto_recruit_task_ptr->set_max_times(times)
            .set_need_refresh(refresh)
            .set_use_expedited(expedite)
            .set_select_level(std::move(select))
            .set_confirm_level(std::move(confirm));
    }

    return true;
}
