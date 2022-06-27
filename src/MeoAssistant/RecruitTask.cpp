#include "RecruitTask.h"

#include "AutoRecruitTask.h"
#include "RecruitCalcTask.h"
#include "ProcessTask.h"

asst::RecruitTask::RecruitTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_recruit_begin_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType)),
    m_auto_recruit_task_ptr(std::make_shared<AutoRecruitTask>(callback, callback_arg, TaskType)),
    m_recruit_only_calc_task_ptr(std::make_shared<RecruitCalcTask>(callback, callback_arg, TaskType))
{
    m_recruit_begin_task_ptr->set_tasks({ "RecruitBegin" });
    m_recruit_only_calc_task_ptr->set_enable(false);

    m_subtasks.emplace_back(m_recruit_begin_task_ptr);
    m_subtasks.emplace_back(m_auto_recruit_task_ptr)->set_retry_times(5);
    m_subtasks.emplace_back(m_recruit_only_calc_task_ptr);
}

bool asst::RecruitTask::set_params(const json::value& params)
{
    auto select_opt = params.find<json::array>("select");
    auto confirm_opt = params.find<json::array>("confirm");
    if (!select_opt || !confirm_opt) {
        return false;
    }

    std::vector<int> select;
    for (const auto& select_num_json : select_opt.value()) {
        if (!select_num_json.is_number()) {
            return false;
        }
        select.emplace_back(select_num_json.as_integer());
    }

    std::vector<int> confirm;
    for (const auto& confirm_num_json : confirm_opt.value()) {
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
    bool skip_robot = params.get("skip_robot", true);

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
            .set_confirm_level(std::move(confirm))
            .set_skip_robot(skip_robot);
    }

    return true;
}
