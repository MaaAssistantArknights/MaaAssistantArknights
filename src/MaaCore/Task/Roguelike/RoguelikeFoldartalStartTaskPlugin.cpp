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

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    if (m_config->get_theme() != RoguelikeTheme::Sami || m_config->get_difficulty() != INT_MAX ||
        !m_config->get_start_foldartal()) {
        return false;
    }
    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;

    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }    
    if (task_view == "Roguelike@LastReward" || task_view == "Roguelike@LastReward4") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeFoldartalStartTaskPlugin::_run()
{

    LogTraceFunction;

    auto mode = m_config->get_mode();
    bool start_foldartal_checked = check_foldartals();

    // 没有刷到需要的板子，退出重开
    if (mode == RoguelikeMode::Collectible && !start_foldartal_checked) {
        m_config->set_difficulty(0);
        ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ExitThenAbandon" })
            .set_times_limit("Roguelike@Abandon", 0)
            .run();
    }
    return true;
}

bool asst::RoguelikeFoldartalStartTaskPlugin::check_foldartals()
{
    // 用户输入的需要的板子字符串
    std::string start_foldartal_string = m_config->get_start_foldartal_list();
    std::vector<std::string> all_foldartal = m_config->get_foldartal();
    std::istringstream iss(start_foldartal_string);
    std::string item;
    std::vector<std::string> extracted_foldartals;
    int count = 0;

    // 分割字符串，提取元素
    while (std::getline(iss, item, ';') && count < 3) {
        extracted_foldartals.push_back(item);
        ++count;
    }

    // 查找板子
    for (const auto& foldartal : extracted_foldartals) {
        if (std::find(all_foldartal.begin(), all_foldartal.end(), foldartal) == all_foldartal.end()) {
            // 如果板子没有找到，返回false
            return false;
        }
    }

    // 如果所有板子都找到了，停止
    return true;
}
