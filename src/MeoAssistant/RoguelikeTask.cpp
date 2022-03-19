#include "RoguelikeTask.h"

#include "ProcessTask.h"
#include "RoguelikeFormationTaskPlugin.h"
#include "RoguelikeBattleTaskPlugin.h"
#include "RoguelikeRecruitTaskPlugin.h"
#include "RoguelikeSkillSelectionTaskPlugin.h"
#include "RoguelikeBattleTaskPlugin.h"

#include "Logger.hpp"

asst::RoguelikeTask::RoguelikeTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_roguelike_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType))
{
    m_roguelike_task_ptr->set_tasks({ "Roguelike1Begin" })
        .set_retry_times(50);

    m_roguelike_task_ptr->regiseter_plugin<RoguelikeFormationTaskPlugin>();

    m_battle_task_ptr = m_roguelike_task_ptr->regiseter_plugin<RoguelikeBattleTaskPlugin>();

    m_recruit_task_ptr = m_roguelike_task_ptr->regiseter_plugin<RoguelikeRecruitTaskPlugin>();
    m_recruit_task_ptr->set_retry_times(2);

    m_skill_task_ptr = m_roguelike_task_ptr->regiseter_plugin<RoguelikeSkillSelectionTaskPlugin>();
    m_skill_task_ptr->set_retry_times(0);

    // 这个任务如果卡住会放弃当前的肉鸽并重新开始，所以多添加一点。先这样凑合用
    for (int i = 0; i != 10000; ++i) {
        m_subtasks.emplace_back(m_roguelike_task_ptr);
    }
}

bool asst::RoguelikeTask::set_params(const json::value& params)
{
    // 0 - 尽可能一直往后打
    // 1 - 第一层投资完源石锭就退出
    // 2 - 投资过后再退出，没有投资就继续往后打

    if (!params.contains("opers") || !params.at("opers").is_array()) {
        return false;
    }

    std::vector<std::string> opers_vec;
    RoguelikeSkillSelectionTaskPlugin::SkillMap skill_map;
    RoguelikeBattleTaskPlugin::SkillUsageMap usage_map;

    for (auto& oper : params.at("opers").as_array()) {
        if (!oper.contains("name") || !oper.at("name").is_string()) {
            return false;
        }
        int skill = oper.get("skill", 1);
        if (skill < 1 || 3 < skill) {
            return false;
        }
        std::string name = oper.at("name").as_string();
        opers_vec.emplace_back(name);
        skill_map.emplace(name, skill);

        auto usage = static_cast<BattleSkillUsage>(oper.get("skill_usage", 0));
        usage_map.emplace(name, usage);
    }
    skill_map.emplace("Unknown", 3);

    int mode = params.get("mode", 0);
    switch (mode) {
    case 0:
        break;
    case 1:
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderLeave", 0);
        break;
    case 2:
        m_roguelike_task_ptr->set_times_limit("Roguelike1StageTraderInvestCancel", 0);
        break;
    default:
        Log.error(__FUNCTION__, "| Unknown mode", mode);
        return false;
    }

    m_recruit_task_ptr->set_opers(std::move(opers_vec));
    m_skill_task_ptr->set_skill_map(std::move(skill_map));

    m_battle_task_ptr->set_skill_usage(std::move(usage_map));

    return true;
}
