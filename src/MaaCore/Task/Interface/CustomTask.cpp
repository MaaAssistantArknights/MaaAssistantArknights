#include "CustomTask.h"

#include "Config/TaskData.h"
#include "Task/MiniGame/PixelPaintTaskPlugin.h"
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

        if (parse_and_register_secretfront(task_name, resolved_task)) {
            Log.info("Parsed and registered SecretFront task: ", task_name, " -> ", resolved_task);
        }
        else if (parse_and_register_pixel_paint(task_name, params)) {
            Log.info("Parsed and registered PixelPaint task: ", task_name);
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

bool asst::CustomTask::parse_and_register_pixel_paint(const std::string& task_name, const json::value& params)
{
    // 仅当任务名确实是像素画入口时才解析（资源只定义了 @Begin）
    if (task_name != "MiniGame@PixelPaint@Begin") {
        return false;
    }

    auto params_opt = params.find<json::object>("params");
    if (!params_opt) {
        Log.error("set_params failed, params not found for pixel paint");
        return false;
    }
    auto pixel_opt = params_opt->find<json::object>("pixel_paint");
    if (!pixel_opt) {
        Log.error("set_params failed, params.pixel_paint not found");
        return false;
    }

    auto groups_opt = pixel_opt->find<json::array>("groups");
    if (!groups_opt || groups_opt->empty()) {
        Log.error("set_params failed, params.pixel_paint.groups not found");
        return false;
    }

    std::vector<PixelPaintTaskPlugin::Group> groups;
    for (const auto& g : *groups_opt) {
        PixelPaintTaskPlugin::Group group;
        group.color = g.get("color", 0);
        if (group.color < 0 || group.color >= PixelPaintTaskPlugin::Group::PaletteSize) {
            Log.error("set_params failed, pixel paint color out of range:", group.color);
            continue;
        }
        if (auto points_opt = g.find<json::array>("points"); points_opt) {
            for (const auto& p : *points_opt) {
                // 畸形点直接丢弃，避免 as_array 抛异常穿过 C ABI
                if (!p.is_array()) {
                    continue;
                }
                auto arr = p.as_array();
                if (arr.size() < 2 || !arr[0].is_number() || !arr[1].is_number()) {
                    continue;
                }
                const int x = arr[0].as_integer();
                const int y = arr[1].as_integer();
                if (x < 0 || x >= PixelPaintTaskPlugin::Group::GridSize || y < 0 ||
                    y >= PixelPaintTaskPlugin::Group::GridSize) {
                    continue;
                }
                group.points.emplace_back(x, y);
            }
        }
        if (!group.points.empty()) {
            groups.emplace_back(std::move(group));
        }
    }

    if (groups.empty()) {
        Log.error("set_params failed, params.pixel_paint.groups is empty");
        return false;
    }

    auto plugin_ptr = m_custom_task_ptr->register_plugin<PixelPaintTaskPlugin>();
    plugin_ptr->set_groups(std::move(groups));
    plugin_ptr->set_swipe_enabled(pixel_opt->get("swipe", true));
    // grid_delay：每格额外等待；兼容旧键 grid_click_delay
    plugin_ptr->set_grid_delay(static_cast<unsigned>(
        pixel_opt->get("grid_delay", pixel_opt->get("grid_click_delay", 0))));
    Log.info("PixelPaint groups:", plugin_ptr->get_groups().size());
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
