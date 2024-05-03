#include "RecruitTask.h"

#include "Task/Miscellaneous/AutoRecruitTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::RecruitTask::RecruitTask(const AsstCallback& callback, Assistant* inst)
    : InterfaceTask(callback, inst, TaskType),
      m_auto_recruit_task_ptr(std::make_shared<AutoRecruitTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_subtasks.emplace_back(m_auto_recruit_task_ptr)->set_retry_times(5);
}

bool asst::RecruitTask::set_params(const json::value& params)
{
    LogTraceFunction;

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

    ExtraTagsMode extra_tags_mode = static_cast<ExtraTagsMode>(params.get("extra_tags_mode", 0));
    // 若出现未知 extra_tags_mode ， 将 extra_tags_mod 置为默认的 NoExtra 。
    if (!RecruitConfig::is_valid_extra_tags_mode(extra_tags_mode)) {
        extra_tags_mode = ExtraTagsMode::NoExtra;
    }

    bool refresh = params.get("refresh", false);
    bool set_time = params.get("set_time", true);
    bool force_refresh = params.get("force_refresh", true);
    int times = params.get("times", 0);
    bool expedite = params.get("expedite", false);
    [[maybe_unused]] int expedite_times = params.get("expedite_times", 0);
    bool skip_robot = params.get("skip_robot", true);
    std::vector<std::string> first_tags = params.get("first_tags", std::vector<std::string>(0));

    std::unordered_map<int /*level*/, int /*minute*/> recruitment_time_map;
    recruitment_time_map[3] = std::clamp(params.get("recruitment_time", "3", 9 * 60), 1 * 60, 9 * 60);
    recruitment_time_map[4] = std::clamp(params.get("recruitment_time", "4", 9 * 60), 1 * 60, 9 * 60);
    recruitment_time_map[5] = std::clamp(params.get("recruitment_time", "5", 9 * 60), 1 * 60, 9 * 60);
    recruitment_time_map[6] = std::clamp(params.get("recruitment_time", "6", 9 * 60), 1 * 60, 9 * 60);

    bool penguin_enabled = params.get("report_to_penguin", false);
    std::string penguin_id = params.get("penguin_id", "");
    bool yituliu_enabled = params.get("report_to_yituliu", false);
    std::string yituliu_id = params.get("yituliu_id", "");
    std::string server = params.get("server", "CN");

    m_auto_recruit_task_ptr->set_enable(true);

    m_auto_recruit_task_ptr->set_max_times(times)
        .set_need_refresh(refresh)
        .set_use_expedited(expedite)
        .set_select_extra_tags(extra_tags_mode)
        .set_first_tags(first_tags)
        .set_select_level(std::move(select))
        .set_confirm_level(std::move(confirm))
        .set_skip_robot(skip_robot)
        .set_recruitment_time(recruitment_time_map)
        .set_penguin_enabled(penguin_enabled, penguin_id)
        .set_yituliu_enabled(yituliu_enabled)
        .set_server(server)
        .set_set_time(set_time)
        .set_force_refresh(force_refresh)
        .set_retry_times(3);

    return true;
}
