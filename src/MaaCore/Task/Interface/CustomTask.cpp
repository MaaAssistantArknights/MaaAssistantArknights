#include "CustomTask.h"

#include "Config/TaskData.h"
#include "Task/MiniGame/SecretFrontTaskPlugin.h"
#include "Task/Miscellaneous/ScreenshotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

asst::CustomTask::CustomTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_custom_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;
    m_custom_task_ptr->register_plugin<ScreenshotTaskPlugin>();
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

        // 将 SecretFront 特殊处理逻辑拆到独立方法中，set_params 保持简洁
        if (task_name.rfind("MiniGame@SecretFront@", 0) == 0) {
            if (!parse_and_register_secretfront(task_name, resolved_task)) {
                Log.error("set_params failed parsing secretfront task: ", task_name);
                return false;
            }
        }

        if (Task.get(resolved_task) == nullptr) {
            Log.error("set_params failed, task not found: ", resolved_task);
            return false;
        }

        tasks.emplace_back(std::move(resolved_task));
    }
    m_custom_task_ptr->set_tasks(std::move(tasks));
    m_subtasks.emplace_back(m_custom_task_ptr);
    return true;
}

bool asst::CustomTask::parse_and_register_secretfront(const std::string& task_name, std::string& resolved_task)
{
    resolved_task = task_name;
    std::optional<std::string> event_name;
    std::optional<std::string> ending_token;

    // 允许识别形式：MiniGame@SecretFront@Begin@Ending[ABCDE]@[事件名]
    auto pos = task_name.find("@Begin");
    if (pos != std::string::npos) {
        size_t start = pos + strlen("@Begin");
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
            resolved_task = "MiniGame@SecretFront@Begin";
        }

        // 如果存在第二个 '@' 分隔，后续内容视为事件名（直到末尾）
        if (next_at != std::string::npos && next_at + 1 < task_name.size()) {
            event_name = task_name.substr(next_at + 1);
        }
    }

    auto plugin_ptr = m_custom_task_ptr->register_plugin<SecretFrontTaskPlugin>();
    if (event_name && !event_name->empty()) {
        plugin_ptr->set_event_name(*event_name);
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
    }

    return true;
}
