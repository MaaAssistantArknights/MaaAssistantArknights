#include "RoguelikeFoldartalStartTaskPlugin.h"

#include "Config/TaskData.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeFoldartalStartTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_config->get_theme() != RoguelikeTheme::Sami || m_config->get_difficulty() != INT_MAX || !m_start_foldartal) {
        return false;
    }

    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;

    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@LastRewardChoose") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeFoldartalStartTaskPlugin::load_params(const json::value& params)
{
    if (m_config->get_theme() != RoguelikeTheme::Sami) {
        return false;
    }

    // 是否生活队凹开局板子
    m_start_foldartal = (params.contains("start_foldartal_list"));

    if (auto opt = params.find<json::array>("start_foldartal_list"); opt) {
        std::vector<std::string> list;
        for (const auto& name : *opt) {
            if (std::string name_str = name.as_string(); !name_str.empty()) {
                list.emplace_back(name_str);
            }
        }
        /* 由于插件 load_param返回值仅决定自身是否启用，参数验证移动至他处 */
        /*
        if (list.empty()) {
            Log.error(__FUNCTION__, "| Empty start_foldartal_list");
            return false;
        }
        */
        m_start_foldartal_list = (std::move(list));
    }

    return true;
}

bool asst::RoguelikeFoldartalStartTaskPlugin::_run()
{
    LogTraceFunction;

    auto mode = m_config->get_mode();
    bool start_foldartal_checked = check_foldartals();

    // 没有刷到需要的板子，退出重开
    if (mode == RoguelikeMode::Collectible && !start_foldartal_checked) {
        m_config->set_next_difficulty(0);
        Task.set_task_base("Roguelike@LastReward", "Roguelike@LastReward_restart");
        Task.set_task_base("Roguelike@LastReward4", "Roguelike@LastReward_restart");
    }
    return true;
}

bool asst::RoguelikeFoldartalStartTaskPlugin::check_foldartals()
{
    const auto& all_foldartal = m_config->get_foldartal();

    // 查找板子
    for (const auto& foldartal : m_start_foldartal_list) {
        if (std::find(all_foldartal.begin(), all_foldartal.end(), foldartal) == all_foldartal.end()) {
            // 如果板子没有找到，返回false
            return false;
        }
    }

    // 如果所有板子都找到了，停止
    return true;
}
