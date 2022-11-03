#include "TaskData.h"

#include <algorithm>
#include <meojson/json.hpp>

#include "Resource/GeneralConfiger.h"
#include "Resource/TemplResource.h"
#include "Utils/AsstRanges.hpp"
#include "Utils/AsstTypes.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

    const auto& json_obj = json.as_object();

    {
        std::unordered_map<std::string_view, bool> to_be_generated;
        for (const std::string& name : json_obj | views::keys) {
            to_be_generated[task_name_view(name)] = true;
        }

        auto generate_task_and_its_base = [&](const std::string& name) -> bool {
            auto generate_task = [&](const std::string& name, std::string_view prefix, taskptr_t base_ptr,
                                     const json::value& task_json) {
                auto task_info_ptr = generate_task_info(name, task_json, base_ptr, prefix);
                if (task_info_ptr == nullptr) {
                    return false;
                }
                to_be_generated[task_name_view(name)] = false;
                insert_or_assign_raw_task(name, task_info_ptr);
                return true;
            };
            std::function<bool(const std::string&, bool)> generate_fun;
            // must_true 若为真，那么 return false 了就是炸了。
            // 否则可能只是某个 B@A 的任务没定义 A（这不是少见现象，例如 Roguelike@Abandon）
            generate_fun = [&](const std::string& name, bool must_true) -> bool {
                // 不是需要 generate 的资源（不在 tasks.json 中）
                if (!to_be_generated[task_name_view(name)]) {
                    // 已生成（它是之前加载过的某个资源的 base）
                    if (m_raw_all_tasks_info.contains(name)) {
                        return true;
                    }
                    // 不在 json 内且未生成（例如生成 C@B@A 时没有定义 B@A，而是定义了 A）
                    if (size_t p = name.find('@'); p != std::string::npos) {
                        return generate_fun(name.substr(p + 1), must_true);
                    }
                    // 例如 Roguelike@Abandon 在这里如果 must_true = true，会报 `Unknown task: Abandon`
                    if (must_true) {
                        Log.error("Unknown task:", name);
                    }
                    return false;
                }
                const json::value& task_json = json_obj.at(name);

                // 已生成（例如国际服覆写国服资源）
                if (m_raw_all_tasks_info.contains(name)) {
                    return generate_task(name, "", get_raw(name), task_json);
                }

                // BaseTask
                if (auto opt = task_json.find<std::string>("baseTask")) {
                    std::string base = opt.value();
                    return generate_fun(base, must_true) && generate_task(name, "", get_raw(base), task_json);
                }

                // TemplateTask
                if (size_t p = name.find('@'); p != std::string::npos) {
                    if (std::string base = name.substr(p + 1); generate_fun(base, false)) {
                        return generate_task(name, name.substr(0, p), get_raw(base), task_json);
                    }
                    // 这类任务有点多，非必要不输出（
                    // Log.debug("Task", name, "based on unknown task, just use task_json.");
                }
                return generate_task(name, "", nullptr, task_json);
            };
            return generate_fun(name, true);
        };

        for (const std::string& name : json_obj | views::keys) {
            generate_task_and_its_base(name);
        }

        // 生成 # 型任务
        for (const auto& [name, old_task] : m_raw_all_tasks_info) {
            expend_sharp_task(name, old_task);
        }
    }

#ifdef ASST_DEBUG
    {
        bool validity = true;

        // 语法检查
        for (const auto& [name, task_json] : json_obj) [[likely]] {
            validity &= syntax_check(name, task_json);
        }

        for (const auto& [name, task] : m_all_tasks_info) {
            auto check_tasklist = [&](const tasklist_t& task_list, std::string_view list_type,
                                      bool enable_justreturn_check = false) {
                std::unordered_set<std::string_view> tasks_set {};
                std::string justreturn_task_name = "";
                for (const std::string& task_name : task_list) {
                    if (tasks_set.contains(task_name)) [[unlikely]] {
                        continue;
                    }
                    // 检查是否有 JustReturn 任务不是最后一个任务
                    if (enable_justreturn_check && !justreturn_task_name.empty()) [[unlikely]] {
                        Log.error((std::string(name) += "->") += list_type,
                                  "has a not-final JustReturn task:", justreturn_task_name);
                        validity = false;
                    }

                    if (auto ptr = get_raw(task_name); ptr == nullptr) [[unlikely]] {
                        Log.error(task_name, "in", (std::string(name) += "->") += list_type, "is null");
                        validity = false;
                    }
                    else if (ptr->algorithm == AlgorithmType::JustReturn) {
                        justreturn_task_name = ptr->name;
                    }

                    tasks_set.emplace(task_name_view(task_name));
                }

                return true;
            };
            check_tasklist(task->next, "next", true);
            check_tasklist(task->sub, "sub");
            check_tasklist(task->exceeded_next, "exceeded_next", true);
            check_tasklist(task->on_error_next, "on_error_next", true);
            check_tasklist(task->reduce_other_times, "reduce_other_times");
        }

        if (!validity) return false;
    }
#endif
    return true;
}

std::optional<asst::TaskData::taskptr_t> asst::TaskData::expend_sharp_task(std::string_view name, taskptr_t old_task)
{
    if (old_task == nullptr) [[unlikely]] {
        return std::nullopt;
    }
    bool task_changed = false;
    auto task_info = _generate_task_info(old_task);
    auto expend_sharp_task_list = [&](tasklist_t& new_task_list, const tasklist_t& task_list,
                                      std::string_view list_type) -> bool {
        new_task_list.clear();
        std::function<bool(const tasklist_t&)> generate_tasks;
        std::unordered_set<std::string_view> tasks_set {};
        generate_tasks = [&](const tasklist_t& raw_tasks) {
            for (std::string_view task : raw_tasks) {
                if (task.empty()) {
                    Log.error("Task", name, "has a empty", list_type);
                    return false;
                }
                if (tasks_set.contains(task)) [[unlikely]] {
                    task_changed = true;
                    continue;
                }
                tasks_set.emplace(task_name_view(task));

                size_t pos = task.find('#');
                if (pos == std::string_view::npos) [[likely]] {
                    new_task_list.emplace_back(task);
                    continue;
                }
#ifdef ASST_DEBUG
                if (pos == 0) [[unlikely]] {
                    Log.trace("Task", name, "has a virtual", list_type, ":", (std::string("`") += task) += '`');
                }
#endif // ASST_DEBUG

                task_changed = true;
                std::string_view type = task.substr(pos + 1);
                if (type == "self") {
                    new_task_list.emplace_back(name);
                    continue;
                }
                taskptr_t other_task_info_ptr = pos ? get_raw(task.substr(0, pos)) : default_task_info_ptr;
#define ASST_TASKDATA_GENERATE_TASKS(t)                \
    else if (type == #t)                               \
    {                                                  \
        if (!generate_tasks(other_task_info_ptr->t)) { \
            return false;                              \
        }                                              \
    }
                if (other_task_info_ptr == nullptr) [[unlikely]] {
                    Log.error("Task", task, "not found");
                    return false;
                }
                else if (type == "back") {
                    // "A#back" === "A", "B@A#back" === "B@A", "#back" === null
                    if (pos) {
                        new_task_list.emplace_back(task.substr(0, pos));
                    }
                }
                ASST_TASKDATA_GENERATE_TASKS(next)
                ASST_TASKDATA_GENERATE_TASKS(sub)
                ASST_TASKDATA_GENERATE_TASKS(on_error_next)
                ASST_TASKDATA_GENERATE_TASKS(exceeded_next)
                ASST_TASKDATA_GENERATE_TASKS(reduce_other_times)
                else [[unlikely]]
                {
                    Log.error("Unknown type", type, "in", task);
                    return false;
                }
#undef ASST_TASKDATA_GENERATE_TASKS
            }

            return true;
        };
        if (!generate_tasks(task_list)) [[unlikely]] {
            Log.error("Generate task_list", (std::string(name) += "->") += list_type, "failed.");
            return false;
        }
        return true;
    };

#define ASST_TASKDATA_GENERATE_SHARP_TASK(type)                            \
    if (!expend_sharp_task_list(task_info->type, old_task->type, #type)) { \
        return std::nullopt;                                               \
    }
    ASST_TASKDATA_GENERATE_SHARP_TASK(next);
    ASST_TASKDATA_GENERATE_SHARP_TASK(sub);
    ASST_TASKDATA_GENERATE_SHARP_TASK(exceeded_next);
    ASST_TASKDATA_GENERATE_SHARP_TASK(on_error_next);
    ASST_TASKDATA_GENERATE_SHARP_TASK(reduce_other_times);
#undef ASST_TASKDATA_GENERATE_SHARP_TASK

    // tasks 个数超过上限时不再 emplace，返回临时值
    constexpr size_t MAX_TASKS_SIZE = 65535;
    if (m_all_tasks_info.size() < MAX_TASKS_SIZE) [[likely]] {
        return insert_or_assign_task(name, task_changed ? task_info : old_task).first->second;
    }
    else {
#ifdef ASST_DEBUG
        Log.debug("Task count has exceeded the upper limit:", MAX_TASKS_SIZE, "current task:", name);
#endif // ASST_DEBUG
        return task_changed ? task_info : old_task;
    }
}

asst::TaskData::taskptr_t asst::TaskData::generate_task_info(const std::string& name, const json::value& task_json,
                                                             taskptr_t default_ptr, std::string_view task_prefix)
{
    if (default_ptr == nullptr) {
        default_ptr = default_task_info_ptr;
        task_prefix = "";
    }

    // 获取 algorithm 并按照 algorithm 生成 TaskInfo
    auto algorithm = default_ptr->algorithm;
    taskptr_t default_derived_ptr = default_ptr;
    if (auto opt = task_json.find<std::string>("algorithm")) {
        std::string algorithm_str = opt.value();
        algorithm = get_algorithm_type(algorithm_str);
        if (default_ptr->algorithm != algorithm) {
            // 相同 algorithm 时才继承派生类成员
            default_derived_ptr = nullptr;
        }
    }
    taskptr_t task_info_ptr = nullptr;
    switch (algorithm) {
    case AlgorithmType::MatchTemplate:
        task_info_ptr =
            generate_match_task_info(name, task_json, std::dynamic_pointer_cast<MatchTaskInfo>(default_derived_ptr));
        break;
    case AlgorithmType::OcrDetect:
        task_info_ptr =
            generate_ocr_task_info(name, task_json, std::dynamic_pointer_cast<OcrTaskInfo>(default_derived_ptr));
        break;
    case AlgorithmType::Hash:
        task_info_ptr =
            generate_hash_task_info(name, task_json, std::dynamic_pointer_cast<HashTaskInfo>(default_derived_ptr));
        break;
    case AlgorithmType::JustReturn:
        task_info_ptr = std::make_shared<TaskInfo>();
        break;
    default:
        Log.error("Unknown algorithm in task", name);
        return nullptr;
    }

    // 不管什么algorithm，都有基础成员（next, roi, 等等）
    if (!append_base_task_info(task_info_ptr, name, task_json, default_ptr, task_prefix)) {
        return nullptr;
    }
    task_info_ptr->algorithm = algorithm;
    task_info_ptr->name = name;
    return task_info_ptr;
}

asst::TaskData::taskptr_t asst::TaskData::generate_match_task_info(const std::string& name,
                                                                   const json::value& task_json,
                                                                   std::shared_ptr<MatchTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_match_task_info_ptr;
    }
    auto match_task_info_ptr = std::make_shared<MatchTaskInfo>();
    // template 留空时不从模板任务继承
    match_task_info_ptr->templ_name = task_json.get("template", name + ".png");
    m_templ_required.emplace(match_task_info_ptr->templ_name);

    // 其余若留空则继承模板任务
    match_task_info_ptr->templ_threshold = task_json.get("templThreshold", default_ptr->templ_threshold);
    if (auto opt = task_json.find<json::array>("maskRange")) {
        auto& mask_range = *opt;
        match_task_info_ptr->mask_range =
            std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
    }
    else {
        match_task_info_ptr->mask_range = default_ptr->mask_range;
    }
    return match_task_info_ptr;
}

asst::TaskData::taskptr_t asst::TaskData::generate_ocr_task_info([[maybe_unused]] const std::string& name,
                                                                 const json::value& task_json,
                                                                 std::shared_ptr<OcrTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_ocr_task_info_ptr;
    }
    auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();

    auto array_opt = task_json.find<json::array>("text");
    ocr_task_info_ptr->text = array_opt ? to_string_list(array_opt.value()) : default_ptr->text;
#ifdef ASST_DEBUG
    if (!array_opt && default_ptr->text.empty()) {
        Log.warn("Ocr task", name, "has implicit empty text.");
    }
#endif

    ocr_task_info_ptr->full_match = task_json.get("fullMatch", default_ptr->full_match);
    ocr_task_info_ptr->is_ascii = task_json.get("isAscii", default_ptr->is_ascii);
    if (auto opt = task_json.find<json::array>("ocrReplace")) {
        for (const json::value& rep : opt.value()) {
            ocr_task_info_ptr->replace_map.emplace(rep[0].as_string(), rep[1].as_string());
        }
    }
    else {
        ocr_task_info_ptr->replace_map = default_ptr->replace_map;
    }
    return ocr_task_info_ptr;
}

asst::TaskData::taskptr_t asst::TaskData::generate_hash_task_info([[maybe_unused]] const std::string& name,
                                                                  const json::value& task_json,
                                                                  std::shared_ptr<HashTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_hash_task_info_ptr;
    }
    auto hash_task_info_ptr = std::make_shared<HashTaskInfo>();
    auto array_opt = task_json.find<json::array>("hash");
    hash_task_info_ptr->hashes = array_opt ? to_string_list(array_opt.value()) : default_ptr->hashes;
#ifdef ASST_DEBUG
    if (!array_opt && default_ptr->hashes.empty()) {
        Log.warn("Hash task", name, "has implicit empty hashes.");
    }
#endif

    hash_task_info_ptr->dist_threshold = task_json.get("threshold", default_ptr->dist_threshold);

    if (auto opt = task_json.find<json::array>("maskRange")) {
        auto& mask_range = *opt;
        hash_task_info_ptr->mask_range =
            std::make_pair(static_cast<int>(mask_range[0]), static_cast<int>(mask_range[1]));
    }
    else {
        hash_task_info_ptr->mask_range = default_ptr->mask_range;
    }
    hash_task_info_ptr->bound = task_json.get("bound", default_ptr->bound);

    return hash_task_info_ptr;
}

bool asst::TaskData::append_base_task_info(taskptr_t task_info_ptr, const std::string& name,
                                           const json::value& task_json, taskptr_t default_ptr,
                                           std::string_view task_prefix)
{
    if (default_ptr == nullptr) {
        default_ptr = default_task_info_ptr;
    }
    if (auto opt = task_json.find<std::string>("action")) {
        std::string action = opt.value();
        task_info_ptr->action = get_action_type(action);
        if (task_info_ptr->action == ProcessTaskAction::Invalid) [[unlikely]] {
            Log.error("Unknown action:", action, ", Task:", name);
            return false;
        }
    }
    else {
        task_info_ptr->action = default_ptr->action;
    }
    task_info_ptr->cache = task_json.get("cache", default_ptr->cache);
    task_info_ptr->max_times = task_json.get("maxTimes", default_ptr->max_times);
    auto array_opt = task_json.find<json::array>("exceededNext");
    task_info_ptr->exceeded_next =
        array_opt ? to_string_list(array_opt.value()) : append_prefix(default_ptr->exceeded_next, task_prefix);
    array_opt = task_json.find<json::array>("onErrorNext");
    task_info_ptr->on_error_next =
        array_opt ? to_string_list(array_opt.value()) : append_prefix(default_ptr->on_error_next, task_prefix);
    task_info_ptr->pre_delay = task_json.get("preDelay", default_ptr->pre_delay);
    task_info_ptr->post_delay = task_json.get("postDelay", default_ptr->post_delay);
    array_opt = task_json.find<json::array>("reduceOtherTimes");
    task_info_ptr->reduce_other_times =
        array_opt ? to_string_list(array_opt.value()) : append_prefix(default_ptr->reduce_other_times, task_prefix);
    if (auto opt = task_json.find<json::array>("roi")) {
        auto& roi_arr = *opt;
        int x = static_cast<int>(roi_arr[0]);
        int y = static_cast<int>(roi_arr[1]);
        int width = static_cast<int>(roi_arr[2]);
        int height = static_cast<int>(roi_arr[3]);
#ifdef ASST_DEBUG
        if (x + width > WindowWidthDefault || y + height > WindowHeightDefault) {
            Log.error(name, "roi is out of bounds");
            return false;
        }
#endif
        task_info_ptr->roi = Rect(x, y, width, height);
    }
    else {
        task_info_ptr->roi = default_ptr->roi;
    }
    array_opt = task_json.find<json::array>("sub");
    task_info_ptr->sub = array_opt ? to_string_list(array_opt.value()) : append_prefix(default_ptr->sub, task_prefix);
    task_info_ptr->sub_error_ignored = task_json.get("subErrorIgnored", default_ptr->sub_error_ignored);
    array_opt = task_json.find<json::array>("next");
    task_info_ptr->next = array_opt ? to_string_list(array_opt.value()) : append_prefix(default_ptr->next, task_prefix);
    if (auto opt = task_json.find<json::array>("rectMove")) {
        auto& move_arr = opt.value();
        task_info_ptr->rect_move = Rect(move_arr[0].as_integer(), move_arr[1].as_integer(), move_arr[2].as_integer(),
                                        move_arr[3].as_integer());
    }
    else {
        task_info_ptr->rect_move = default_ptr->rect_move;
    }

    if (auto opt = task_json.find<json::array>("specificRect")) {
        auto& rect_arr = opt.value();
        task_info_ptr->specific_rect = Rect(rect_arr[0].as_integer(), rect_arr[1].as_integer(),
                                            rect_arr[2].as_integer(), rect_arr[3].as_integer());
    }
    else {
        task_info_ptr->specific_rect = default_ptr->specific_rect;
    }
    if (auto opt = task_json.find<json::array>("specialParams")) {
        auto& special_params = opt.value();
        for (auto& param : special_params) {
            task_info_ptr->special_params.emplace_back(param.as_integer());
        }
    }
    else {
        task_info_ptr->special_params = default_ptr->special_params;
    }
    return true;
}

std::shared_ptr<asst::MatchTaskInfo> asst::TaskData::_default_match_task_info()
{
    auto match_task_info_ptr = std::make_shared<MatchTaskInfo>();
    match_task_info_ptr->templ_name = "__INVALID__";
    match_task_info_ptr->templ_threshold = TemplThresholdDefault;

    return match_task_info_ptr;
}

std::shared_ptr<asst::OcrTaskInfo> asst::TaskData::_default_ocr_task_info()
{
    auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();
    ocr_task_info_ptr->full_match = false;
    ocr_task_info_ptr->is_ascii = false;

    return ocr_task_info_ptr;
}

std::shared_ptr<asst::HashTaskInfo> asst::TaskData::_default_hash_task_info()
{
    auto hash_task_info_ptr = std::make_shared<HashTaskInfo>();
    hash_task_info_ptr->dist_threshold = 0;
    hash_task_info_ptr->bound = true;

    return hash_task_info_ptr;
}

asst::TaskData::taskptr_t asst::TaskData::_default_task_info()
{
    auto task_info_ptr = std::make_shared<TaskInfo>();
    task_info_ptr->algorithm = AlgorithmType::MatchTemplate;
    task_info_ptr->action = ProcessTaskAction::DoNothing;
    task_info_ptr->cache = true;
    task_info_ptr->max_times = INT_MAX;
    task_info_ptr->pre_delay = 0;
    task_info_ptr->post_delay = 0;
    task_info_ptr->roi = Rect();
    task_info_ptr->sub_error_ignored = false;
    task_info_ptr->rect_move = Rect();
    task_info_ptr->specific_rect = Rect();

    return task_info_ptr;
}

#ifdef ASST_DEBUG
// 为了解决类似 beddc7c828126c678391e0b4da288db6d2c2d58a 导致的问题，加载的时候做一个语法检查
// 主要是处理是否包含未知键值的问题
bool asst::TaskData::syntax_check(const std::string& task_name, const json::value& task_json)
{
    static const std::unordered_map<AlgorithmType, std::unordered_set<std::string>> allowed_key_under_algorithm = {
        { AlgorithmType::Invalid,
          {
              "algorithm",       "baseTask",  "template",   "text",         "action",           "sub",
              "subErrorIgnored", "next",      "maxTimes",   "exceededNext", "onErrorNext",      "preDelay",
              "postDelay",       "roi",       "cache",      "rectMove",     "reduceOtherTimes", "templThreshold",
              "maskRange",       "fullMatch", "ocrReplace", "hash",         "specialParams",    "threshold",
          } },
        { AlgorithmType::MatchTemplate,
          {
              "algorithm",
              "baseTask",
              "template",
              "action",
              "sub",
              "subErrorIgnored",
              "next",
              "maxTimes",
              "exceededNext",
              "onErrorNext",
              "preDelay",
              "postDelay",
              "roi",
              "cache",
              "rectMove",
              "reduceOtherTimes",
              "templThreshold",
              "maskRange",
          } },
        { AlgorithmType::OcrDetect,
          {
              "algorithm",
              "baseTask",
              "text",
              "action",
              "sub",
              "subErrorIgnored",
              "isAscii",
              "next",
              "maxTimes",
              "exceededNext",
              "onErrorNext",
              "preDelay",
              "postDelay",
              "roi",
              "cache",
              "rectMove",
              "reduceOtherTimes",
              "fullMatch",
              "ocrReplace",
          } },
        { AlgorithmType::JustReturn,
          { "algorithm", "baseTask", "action", "sub", "subErrorIgnored", "next", "maxTimes", "exceededNext",
            "onErrorNext", "preDelay", "postDelay", "reduceOtherTimes", "specialParams" } },
        { AlgorithmType::Hash,
          {
              "algorithm", "baseTask",  "action",        "sub",         "subErrorIgnored",
              "next",      "maxTimes",  "exceededNext",  "onErrorNext", "preDelay",
              "postDelay", "roi",       "cache",         "rectMove",    "reduceOtherTimes",
              "hash",      "maskRange", "specialParams", "threshold",
          } },
    };

    static const std::unordered_map<ProcessTaskAction, std::unordered_set<std::string>> allowed_key_under_action = {
        { ProcessTaskAction::ClickRect,
          {
              "specificRect",
          } },
        { ProcessTaskAction::Swipe, { "specificRect", "rectMove" } },
    };

    auto is_doc = [&](std::string_view key) {
        return key.find("Doc") != std::string_view::npos || key.find("doc") != std::string_view::npos;
    };

    // 兜底策略，如果某个 key ("xxx") 不符合规范（可能是代码中使用到的参数，而不是任务流程）
    // 需要加一个注释 ("xxx_Doc") 就能过 syntax_check.
    auto has_doc = [&](const std::string& key) -> bool {
        return task_json.find(key + "_Doc") || task_json.find(key + "_doc");
    };

    bool validity = true;
    if (!m_all_tasks_info.contains(task_name)) {
        Log.error("TaskData::syntax_check | Task", task_name, "has not been generated.");
        return false;
    }

    // 获取 algorithm
    auto algorithm = m_all_tasks_info[task_name]->algorithm;
    if (algorithm == AlgorithmType::Invalid) [[unlikely]] {
        Log.error(task_name, "has unknown algorithm.");
        validity = false;
    }

    // 获取 action
    auto action = m_all_tasks_info[task_name]->action;
    if (action == ProcessTaskAction::Invalid) [[unlikely]] {
        Log.error(task_name, "has unknown action.");
        validity = false;
    }

    std::unordered_set<std::string> allowed_key {};
    if (allowed_key_under_algorithm.contains(algorithm)) {
        decltype(allowed_key) tmp = allowed_key_under_algorithm.at(algorithm);
        allowed_key.merge(tmp);
    }
    if (allowed_key_under_action.contains(action)) {
        decltype(allowed_key) tmp = allowed_key_under_action.at(action);
        allowed_key.merge(tmp);
    }

    for (const auto& name : task_json.as_object() | views::keys) {
        if (!allowed_key.contains(name) && !is_doc(name) && !has_doc(name)) {
            Log.error(task_name, "has unknown key:", name);
            validity = false;
        }
    }
    return validity;
}
#endif
