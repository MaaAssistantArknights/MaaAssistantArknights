#include "CustomTask.h"

#include "Config/TaskData.h"
#include "Task/MiniGame/SecretFrontTaskPlugin.h"
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/JieGarden/RoguelikeCoppersRecastPlugin.h"
#include "Utils/Logger.hpp"

asst::CustomTask::CustomTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_custom_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;
    m_custom_task_ptr->register_plugin<ScreenshotTaskPlugin>();

    m_coppers_recast_plugin_ptr = m_custom_task_ptr->register_plugin<RoguelikeCoppersRecastPlugin>();
}

bool asst::CustomTask::set_params(const json::value& params)
{
    LogTraceFunction;

    auto tasks_opt = params.find<json::array>("task_names");
    if (!tasks_opt) {
        Log.error("set_params failed, task_names not found");
        return false;
    }
    std::vector<std::string> tasks;

    for (const auto& t : *tasks_opt) {
        if (!t.is_string()) {
            Log.error("set_params failed, task is not string");
            return false;
        }

        std::string task_name = t.as_string();
        std::string resolved_task = task_name;

        if (parse_and_register_secretfront(task_name, resolved_task)) {
            Log.info("Parsed and registered SecretFront task: ", task_name, " -> ", resolved_task);
        }
        /* else if (parse_and_register_xxx) */
        else {
            resolved_task = task_name;
        }

        if (Task.get(resolved_task) == nullptr) {
            Log.error("set_params failed, task not found: ", resolved_task);
            return false;
        }

        tasks.emplace_back(std::move(resolved_task));
    }
    m_custom_task_ptr->set_tasks(std::move(tasks));

    // 传递参数给插件
    if (m_coppers_recast_plugin_ptr) {
        m_coppers_recast_plugin_ptr->load_params(params);
    }

    m_subtasks.emplace_back(m_custom_task_ptr);
    return true;
}

bool asst::CustomTask::parse_and_register_secretfront(const std::string& task_name, std::string& resolved_task)
{
    if (!task_name.starts_with("MiniGame@SecretFront@Begin")) {
        return false;
    }

    std::optional<std::string> event_name;
    std::optional<std::string> ending_token;

    // 允许识别形式：MiniGame@SecretFront@Begin@Ending[A-E](@(支援作战平台|游侠|诡影迷踪))?
    size_t start = strlen("MiniGame@SecretFront@Begin");
    if (start < task_name.size() && task_name[start] == '@') {
        ++start; // skip the separator '@'
    }

    // 提取 Ending token（直到下一个 '@' 或末尾）
    size_t next_at = task_name.find('@', start);
    std::string ending;
    if (start < task_name.size()) {
        if (next_at == std::string::npos) {
            ending = task_name.substr(start);
        }
        else {
            ending = task_name.substr(start, next_at - start);
        }
    }

    if (!ending.empty()) {
        ending_token = ending;
    }

    // 如果存在第二个 '@' 分隔，后续内容视为事件名（直到末尾）
    if (next_at != std::string::npos && next_at + 1 < task_name.size()) {
        event_name = task_name.substr(next_at + 1);
    }

    auto plugin_ptr = m_custom_task_ptr->register_plugin<SecretFrontTaskPlugin>();
    if (!plugin_ptr) {
        Log.error("Failed to register SecretFrontTaskPlugin");
        return false;
    }

    if (event_name && !event_name->empty()) {
        plugin_ptr->set_event_name(*event_name);
        Log.info("Set SecretFront event name:", *event_name);
    }
    if (ending_token && !ending_token->empty()) {
        if (*ending_token == "EndingA") {
            plugin_ptr->set_ending(SecretFrontTaskPlugin::Ending::A);
        }
        else if (*ending_token == "EndingB") {
            plugin_ptr->set_ending(SecretFrontTaskPlugin::Ending::B);
        }
        else if (*ending_token == "EndingC") {
            plugin_ptr->set_ending(SecretFrontTaskPlugin::Ending::C);
        }
        else if (*ending_token == "EndingD") {
            plugin_ptr->set_ending(SecretFrontTaskPlugin::Ending::D);
        }
        else if (*ending_token == "EndingE") {
            plugin_ptr->set_ending(SecretFrontTaskPlugin::Ending::E);
        }
        Log.info("Set SecretFront ending:", *ending_token);
    }

    resolved_task = "MiniGame@SecretFront@Begin";

    return true;
}
