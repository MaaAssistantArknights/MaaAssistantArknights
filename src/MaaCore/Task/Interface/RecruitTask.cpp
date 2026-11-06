#include "RecruitTask.h"

#include "Task/Miscellaneous/AutoRecruitTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::RecruitTask::RecruitTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
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

    auto oper_priority_mode = static_cast<OperPriorityMode>(params.get("oper_priority_mode", 0));
    if (!RecruitConfig::is_valid_oper_priority_mode(oper_priority_mode)) {
        oper_priority_mode = OperPriorityMode::Disabled;
    }
    // 是否允许为了未拥有/未满潜而招募小车（1 星）和 2 星干员（会缩短该次招募时长）
    bool oper_priority_allow_low_star = params.get("oper_priority_allow_low_star", false);
    // 为小车/2 星干员选中的组合是否自动确认（不受 3/4 星确认设置影响）
    bool oper_priority_low_star_confirm = params.get("oper_priority_low_star_confirm", false);
    // 干员识别数据（GUI 落盘的 OperBoxData.json）路径，仅在 oper_priority_mode 非 0 时使用
    std::string operbox_data_path = params.get("operbox_data_path", std::string());

    bool refresh = params.get("refresh", false);
    bool set_time = params.get("set_time", true);
    bool force_refresh = params.get("force_refresh", true);
    int level3_recruitment_permit_reserve = params.get("level3_recruitment_permit_reserve", 0);
    int times = params.get("times", 0);
    bool expedite = params.get("expedite", false);
    [[maybe_unused]] int expedite_times = params.get("expedite_times", 0);
    std::vector<RecruitConfig::TagId> preserve_tags;
    std::vector<std::string> first_tags = params.get("first_tags", std::vector<std::string>(0));

    if (auto preserve_tags_opt = params.find<std::vector<RecruitConfig::TagId>>("preserve_tags")) {
        preserve_tags.swap(*preserve_tags_opt);
    }
    else if (auto skip_robot_opt = params.find<bool>("skip_robot")) {
        LogWarn << "================  DEPRECATED  ================";
        LogWarn << __FUNCTION__ << " 'skip_robot' has been deprecated in 6.11.0, please use 'preserve_tags' instead.";
        LogWarn << "================  DEPRECATED  ================";
        if (*skip_robot_opt) {
            preserve_tags = { "支援机械" };
        }
    }

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
        .set_preserve_tags(std::move(preserve_tags))
        .set_select_level(std::move(select))
        .set_confirm_level(std::move(confirm))
        .set_level3_recruitment_permit_reserve(level3_recruitment_permit_reserve)
        .set_recruitment_time(recruitment_time_map)
        .set_penguin_enabled(penguin_enabled, penguin_id)
        .set_yituliu_enabled(yituliu_enabled, yituliu_id)
        .set_server(server)
        .set_set_time(set_time)
        .set_force_refresh(force_refresh)
        .set_retry_times(3);

    // 运行中收到的参数更新不改动 ｢优先未拥有/未满潜｣ 相关设置：
    // 子任务运行时会读取干员数据快照，此时替换会与公招计算产生数据竞争；新设置在下一次运行该任务时生效
    if (!m_running) {
        m_auto_recruit_task_ptr->set_oper_priority(
            oper_priority_mode,
            oper_priority_allow_low_star,
            oper_priority_low_star_confirm,
            std::move(operbox_data_path));
    }

    return true;
}
