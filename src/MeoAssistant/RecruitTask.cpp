#include "RecruitTask.h"

#include "AutoRecruitTask.h"
#include "ProcessTask.h"

asst::RecruitTask::RecruitTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_auto_recruit_task_ptr(std::make_shared<AutoRecruitTask>(callback, callback_arg, TaskType))
{
    m_subtasks.emplace_back(m_auto_recruit_task_ptr)->set_retry_times(5);
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


    m_auto_recruit_task_ptr->set_enable(true);

    m_auto_recruit_task_ptr->set_max_times(times)
            .set_need_refresh(refresh)
            .set_use_expedited(expedite)
            .set_select_level(std::move(select))
            .set_confirm_level(std::move(confirm))
            .set_skip_robot(skip_robot)
            .set_set_time(set_time).set_retry_times(3);

    return true;
}
