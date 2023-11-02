#include "TaskData.h"

#include <algorithm>
#include <meojson/json.hpp>

#ifdef ASST_DEBUG
#include <queue>
#endif

#include "Common/AsstTypes.h"
#include "GeneralConfig.h"
#include "TaskData/TaskDataSymbolStream.h"
#include "TaskData/TaskDataTypes.hpp"
#include "TemplResource.h"
#include "Utils/JsonMisc.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}
asst::TaskData::taskptr_t asst::TaskData::get_raw(std::string_view name)
{
    if (!generate_task_and_its_base(name, true)) [[unlikely]] {
        return nullptr;
    }

    if (auto it = m_raw_all_tasks_info.find(name); it != m_raw_all_tasks_info.cend()) {
        return it->second;
    }

    // `@` 前面的字符长度
    size_t name_len = name.find('@');
    if (name_len == std::string_view::npos) [[unlikely]] {
        return nullptr;
    }

    // 隐式 TemplateTask
    if (auto base_task_iter = get_raw(name.substr(name_len + 1))) {
        return _generate_task_info(base_task_iter, name.substr(0, name_len));
    }

    return nullptr;
}

asst::TaskData::taskptr_t asst::TaskData::get(std::string_view name)
{
    // 普通任务 或 已经生成过的高级任务
    if (auto it = m_all_tasks_info.find(name); it != m_all_tasks_info.cend()) [[likely]] {
        return it->second;
    }

    auto raw_task = get_raw(name);
    auto task = _generate_task_info(raw_task);
    if (!task) [[unlikely]] {
        return nullptr;
    }
    bool changed = false; // 任务列表是否发生变化（是否包含虚任务）
#define ASST_TASKDATA_GET_IF_BRANCH(list, m)                                                    \
    if (auto opt = compile_tasklist(raw_task->list, name, m); !opt) [[unlikely]] {              \
        Log.error("Generate task_list", std::string(name) + "->" #list, "failed.", opt.what()); \
        return nullptr;                                                                         \
    }                                                                                           \
    else {                                                                                      \
        changed |= opt.value().task_changed;                                                    \
        task->list = std::move(opt.value().tasks);                                              \
    }
    // 展开五个任务列表中的虚任务
    ASST_TASKDATA_GET_IF_BRANCH(next, false);
    ASST_TASKDATA_GET_IF_BRANCH(sub, true);
    ASST_TASKDATA_GET_IF_BRANCH(exceeded_next, false);
    ASST_TASKDATA_GET_IF_BRANCH(on_error_next, false);
    ASST_TASKDATA_GET_IF_BRANCH(reduce_other_times, true);
#undef ASST_TASKDATA_GET_IF_BRANCH

    constexpr size_t MAX_TASKS_SIZE = 65535;
    if (m_all_tasks_info.size() < MAX_TASKS_SIZE) [[likely]] {
        // 保存最终生成的任务，下次查询时可以直接返回
        return insert_or_assign_task(name, changed ? task : raw_task).first->second;
    }
    else {
        // 个数超过上限时不保存，直接返回，防止内存占用过大
        Log.warn("Task count has exceeded the upper limit:", MAX_TASKS_SIZE, "current task:", name);
        return changed ? task : raw_task;
    }
}

bool asst::TaskData::lazy_parse(const json::value& json)
{
    LogTraceFunction;

    if (!json.is_object()) {
        Log.error("parameter json is not a json::object");
        return false;
    }

    for (const auto& [name, task_json] : json.as_object()) {
        std::string_view name_view = task_name_view(name);
        if (task_json.contains("baseTask")) {
            // 直接声明 baseTask 的任务不继承同名任务参数而是直接覆盖
            m_json_all_tasks_info[name_view] = task_json.as_object();
            std::string base_task = task_json.get("baseTask", "");
#ifdef ASST_DEBUG
            if (base_task.empty()) {
                Log.error("Task", name, "has empty baseTask");
            }
#endif
            if (base_task == "#none") {
                m_json_all_tasks_info[name_view].erase("baseTask");
            }
            continue;
        }
        if (m_json_all_tasks_info.find(name_view) == m_json_all_tasks_info.cend()) {
            m_json_all_tasks_info.emplace(name_view, task_json.as_object());
            continue;
        }
        for (const auto& [key, value] : task_json.as_object()) {
            m_json_all_tasks_info[name_view][key] = value;
        }
    }

    clear_tasks();

#ifdef ASST_DEBUG
    {
        bool validity = true;
        std::queue<std::string_view> task_queue;
        std::unordered_set<std::string_view> checking_task_set;

        for (std::string_view name : m_json_all_tasks_info | views::keys) {
            m_task_status[name] = ToBeGenerate;
            task_queue.push(name);
            checking_task_set.insert(name);
        }

        for (const auto& [name, task_json] : m_json_all_tasks_info) [[likely]] {
            validity &= syntax_check(name, task_json);
        }

        const size_t MAX_CHECKING_SIZE = 10000;
        while (!task_queue.empty() && checking_task_set.size() <= MAX_CHECKING_SIZE) {
            std::string_view name = task_queue.front();
            task_queue.pop();
            auto task = get(name);
            if (task == nullptr) [[unlikely]] {
                Log.error("Task", name, "not successfully generated");
                validity = false;
                continue;
            }
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
                        justreturn_task_name = "";
                        validity = false;
                    }

                    if (auto ptr = get_raw(task_name); ptr == nullptr) [[unlikely]] {
                        Log.error(task_name, "in", (std::string(name) += "->") += list_type, "is null");
                        validity = false;
                        continue;
                    }
                    else if (ptr->algorithm == AlgorithmType::JustReturn) {
                        justreturn_task_name = ptr->name;
                    }

                    if (!checking_task_set.contains(task_name)) {
                        task_queue.emplace(task_name_view(task_name));
                        checking_task_set.emplace(task_name_view(task_name));
                    }
                    tasks_set.emplace(task_name_view(task_name));
                }

                return true;
            };
            bool enable_justreturn_check = true;
            if (name.ends_with("LoadingText") || name.ends_with("LoadingIcon")) {
                // 放宽对 Loading 类任务的限制，不然 JustReturn 的任务如果本身有 JR 的 next，就不能 @Loading 了
                enable_justreturn_check = false;
            }
            check_tasklist(task->next, "next", enable_justreturn_check);
            check_tasklist(task->sub, "sub");
            check_tasklist(task->exceeded_next, "exceeded_next", enable_justreturn_check);
            check_tasklist(task->on_error_next, "on_error_next", enable_justreturn_check);
            check_tasklist(task->reduce_other_times, "reduce_other_times");
        }
        if (checking_task_set.size() > MAX_CHECKING_SIZE) {
            // 生成超出上限一般是出现了会导致无限隐式生成的任务。比如 "#self@LoadingText". 这里给个警告.
            Log.warn("Generating exceeded limit when syntax_check.");
        }
        else {
            Log.trace(checking_task_set.size(), "tasks checked.");
        }
        clear_tasks();
        if (!validity) return false;
    }
#endif

    return true;
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

    if (!lazy_parse(json)) return false;

    // 本来重构之后完全支持惰性加载，但是发现模板图片不支持（
    for (std::string_view name : m_json_all_tasks_info | views::keys) {
        generate_task_and_its_base(name, true);
    }

    return true;
}

void asst::TaskData::clear_tasks()
{
    // 注意：这会导致已经通过 get 获取的任务指针内容不会更新
    // 即运行期修改对已经获取的任务指针无效，但是不会导致崩溃；要想更新，需要重新获取任务指针
    m_all_tasks_info.clear();
    m_raw_all_tasks_info.clear();
    for (std::string_view name : m_json_all_tasks_info | views::keys) {
        m_task_status[task_name_view(name)] = ToBeGenerate;
    }
}

void asst::TaskData::set_task_base(const std::string_view task_name, std::string base_task_name)
{
    m_json_all_tasks_info[task_name_view(task_name)]["baseTask"] = std::move(base_task_name);
    clear_tasks();
}

bool asst::TaskData::generate_task_and_its_base(std::string_view name, bool must_true)
{
    auto generate_task = [&](std::string_view name, std::string_view prefix, taskptr_t base_ptr,
                             const json::value& task_json) {
        auto task_info_ptr = generate_task_info(name, task_json, base_ptr, prefix);
        if (task_info_ptr == nullptr) {
            return false;
        }
        m_task_status[task_name_view(name)] = NotToBeGenerate;
        // 保存虚任务未展开的任务
        insert_or_assign_raw_task(name, task_info_ptr);
        return true;
    };
    switch (m_task_status[task_name_view(name)]) {
    case NotToBeGenerate:
        // 已经显式生成
        if (m_raw_all_tasks_info.contains(name)) {
            return true;
        }

        // 隐式生成的资源
        if (size_t p = name.find('@'); p != std::string::npos) {
            return generate_task_and_its_base(name.substr(p + 1), must_true);
        }

        m_task_status[name] = NotExists;
        [[fallthrough]];
    case NotExists:
        if (must_true) {
            // 必须有名字为 name 的资源
            Log.error("Unknown task:", name);
        }
        // 不一定必须有名字为 name 的资源，例如 Roguelike@Abandon 不必有 Abandon.
        return false;
    case ToBeGenerate: {
        if (!m_json_all_tasks_info.contains(name)) [[unlikely]] {
            // 这段正常情况来说是不可能的，除非有 string_view 引用失效
            Log.error("Unexcepted ToBeGenerate task:", name);
            return false;
        }

        m_task_status[name] = Generating;

        const json::value& task_json = m_json_all_tasks_info.at(name);

        // BaseTask
        if (auto opt = task_json.find<std::string>("baseTask")) {
            std::string base = opt.value();
            return generate_task_and_its_base(base, must_true) && generate_task(name, "", get_raw(base), task_json);
        }

        // TemplateTask
        if (size_t p = name.find('@'); p != std::string::npos) {
            if (std::string_view base = name.substr(p + 1); generate_task_and_its_base(base, false)) {
#ifdef ASST_DEBUG
                if (task_json.as_object().empty() && get_raw(base)->algorithm != AlgorithmType::MatchTemplate) {
                    // 多余的空任务
                    Log.warn("Task", name, "is a redundant empty task because it is not MatchTemplate");
                }
#endif
                return generate_task(name, name.substr(0, p), get_raw(base), task_json);
            }
        }
        return generate_task(name, "", nullptr, task_json);
    }
    [[unlikely]] case Generating:
        Log.error("Task", name, "is generated cyclically");
        return false;
    [[unlikely]] default:
        Log.error("Task", name, "has unknown status");
        return false;
    }
}

asst::TaskData::taskptr_t asst::TaskData::generate_task_info(std::string_view name, const json::value& task_json,
                                                             taskptr_t default_ptr, std::string_view task_prefix)
{
    if (default_ptr == nullptr) {
        default_ptr = default_task_info_ptr;
        task_prefix = "";
    }

    // 获取 algorithm 并按照 algorithm 生成 TaskInfo
    AlgorithmType algorithm;
    utils::get_value_or(name, task_json, "algorithm", algorithm, default_ptr->algorithm);

    taskptr_t task_ptr = nullptr;
    switch (algorithm) {
    case AlgorithmType::MatchTemplate:
        task_ptr = generate_match_task_info(name, task_json, std::dynamic_pointer_cast<MatchTaskInfo>(default_ptr));
        break;
    case AlgorithmType::OcrDetect:
        task_ptr = generate_ocr_task_info(name, task_json, std::dynamic_pointer_cast<OcrTaskInfo>(default_ptr));
        break;
    case AlgorithmType::Hash:
        task_ptr = generate_hash_task_info(name, task_json, std::dynamic_pointer_cast<HashTaskInfo>(default_ptr));
        break;
    case AlgorithmType::JustReturn:
        task_ptr = std::make_shared<TaskInfo>();
        break;
    default:
        Log.error("Unknown algorithm in task", name);
        return nullptr;
    }

    if (task_ptr == nullptr) {
        return nullptr;
    }

    // 不管什么algorithm，都有基础成员（next, roi, 等等）
    if (!append_base_task_info(task_ptr, name, task_json, default_ptr, task_prefix)) {
        return nullptr;
    }
    task_ptr->algorithm = algorithm;
    task_ptr->name = name;
    return task_ptr;
}

asst::TaskData::taskptr_t asst::TaskData::generate_match_task_info(std::string_view name, const json::value& task_json,
                                                                   std::shared_ptr<MatchTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_match_task_info_ptr;
    }
    auto match_task_info_ptr = std::make_shared<MatchTaskInfo>();
    if (!utils::get_value_or(name, task_json, "template", match_task_info_ptr->templ_names,
                             [&]() { return std::vector { std::string(name) + ".png" }; })) {
        return nullptr;
    }

    m_templ_required.insert(match_task_info_ptr->templ_names.begin(), match_task_info_ptr->templ_names.end());

    // 其余若留空则继承模板任务

    auto threshold_opt = task_json.find("templThreshold");
    if (!threshold_opt) {
        match_task_info_ptr->templ_thresholds = default_ptr->templ_thresholds;
        match_task_info_ptr->templ_thresholds.resize(match_task_info_ptr->templ_names.size(),
                                                     default_ptr->templ_thresholds.back());
    }
    else if (threshold_opt->is_number()) {
        // 单个数值时，所有模板都使用这个阈值
        match_task_info_ptr->templ_thresholds.resize(match_task_info_ptr->templ_names.size(),
                                                     threshold_opt->as_double());
    }
    else if (threshold_opt->is_array()) {
        ranges::copy(threshold_opt->as_array() | views::transform(&ranges::range_value_t<json::array>::as_double),
                     std::back_inserter(match_task_info_ptr->templ_thresholds));
    }
    else {
        Log.error("Invalid templThreshold type in task", name);
        return nullptr;
    }

    if (match_task_info_ptr->templ_names.size() != match_task_info_ptr->templ_thresholds.size()) {
        Log.error("Template count and templThreshold count not match in task", name);
        return nullptr;
    }

    if (match_task_info_ptr->templ_names.size() == 0 || match_task_info_ptr->templ_thresholds.size() == 0) {
        Log.error("Template or templThreshold is empty in task", name);
        return nullptr;
    }

    utils::get_value_or(name, task_json, "maskRange", match_task_info_ptr->mask_range, default_ptr->mask_range);
    return match_task_info_ptr;
}

asst::TaskData::taskptr_t asst::TaskData::generate_ocr_task_info(std::string_view name, const json::value& task_json,
                                                                 std::shared_ptr<OcrTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_ocr_task_info_ptr;
    }
    auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();

    // text 不允许为字符串，必须是字符串数组，不能用 utils::get_value_or
    auto array_opt = task_json.find<json::array>("text");
    ocr_task_info_ptr->text = array_opt ? to_string_list(array_opt.value()) : default_ptr->text;
#ifdef ASST_DEBUG
    if (!array_opt && default_ptr->text.empty()) {
        Log.warn("Ocr task", name, "has implicit empty text.");
    }
#endif
    utils::get_value_or(name, task_json, "fullMatch", ocr_task_info_ptr->full_match, default_ptr->full_match);
    utils::get_value_or(name, task_json, "isAscii", ocr_task_info_ptr->is_ascii, default_ptr->is_ascii);
    utils::get_value_or(name, task_json, "withoutDet", ocr_task_info_ptr->without_det, default_ptr->without_det);
    utils::get_value_or(name, task_json, "replaceFull", ocr_task_info_ptr->replace_full, default_ptr->replace_full);
    utils::get_value_or(name, task_json, "ocrReplace", ocr_task_info_ptr->replace_map, default_ptr->replace_map);
    return ocr_task_info_ptr;
}

asst::TaskData::taskptr_t asst::TaskData::generate_hash_task_info(std::string_view name, const json::value& task_json,
                                                                  std::shared_ptr<HashTaskInfo> default_ptr)
{
    if (default_ptr == nullptr) {
        default_ptr = default_hash_task_info_ptr;
    }
    auto hash_task_info_ptr = std::make_shared<HashTaskInfo>();
    // hash 不允许为字符串，必须是字符串数组，不能用 utils::get_value_or
    auto array_opt = task_json.find<json::array>("hash");
    hash_task_info_ptr->hashes = array_opt ? to_string_list(array_opt.value()) : default_ptr->hashes;
#ifdef ASST_DEBUG
    if (!array_opt && default_ptr->hashes.empty()) {
        Log.warn("Hash task", name, "has implicit empty hashes.");
    }
#endif
    utils::get_value_or(name, task_json, "threshold", hash_task_info_ptr->dist_threshold, default_ptr->dist_threshold);
    utils::get_value_or(name, task_json, "maskRange", hash_task_info_ptr->mask_range, default_ptr->mask_range);
    utils::get_value_or(name, task_json, "bound", hash_task_info_ptr->bound, default_ptr->bound);
    return hash_task_info_ptr;
}

bool asst::TaskData::append_base_task_info(taskptr_t task_info_ptr, std::string_view name, const json::value& task_json,
                                           taskptr_t default_ptr, std::string_view task_prefix)
{
    if (default_ptr == nullptr) {
        default_ptr = default_task_info_ptr;
    }

    utils::get_value_or(name, task_json, "action", task_info_ptr->action, default_ptr->action);
    utils::get_value_or(name, task_json, "cache", task_info_ptr->cache, default_ptr->cache);
    utils::get_value_or(name, task_json, "maxTimes", task_info_ptr->max_times, default_ptr->max_times);
    utils::get_value_or(name, task_json, "exceededNext", task_info_ptr->exceeded_next,
                        [&]() { return append_prefix(default_ptr->exceeded_next, task_prefix); });
    utils::get_value_or(name, task_json, "onErrorNext", task_info_ptr->on_error_next,
                        [&]() { return append_prefix(default_ptr->on_error_next, task_prefix); });
    utils::get_value_or(name, task_json, "preDelay", task_info_ptr->pre_delay, default_ptr->pre_delay);
    utils::get_value_or(name, task_json, "postDelay", task_info_ptr->post_delay, default_ptr->post_delay);
    utils::get_value_or(name, task_json, "reduceOtherTimes", task_info_ptr->reduce_other_times,
                        [&]() { return append_prefix(default_ptr->reduce_other_times, task_prefix); });
    utils::get_value_or(name, task_json, "roi", task_info_ptr->roi, default_ptr->roi);
    utils::get_value_or(name, task_json, "sub", task_info_ptr->sub,
                        [&]() { return append_prefix(default_ptr->sub, task_prefix); });
    utils::get_value_or(name, task_json, "subErrorIgnored", task_info_ptr->sub_error_ignored,
                        default_ptr->sub_error_ignored);
    utils::get_value_or(name, task_json, "next", task_info_ptr->next,
                        [&]() { return append_prefix(default_ptr->next, task_prefix); });
    utils::get_value_or(name, task_json, "rectMove", task_info_ptr->rect_move, default_ptr->rect_move);
    utils::get_value_or(name, task_json, "specificRect", task_info_ptr->specific_rect, default_ptr->specific_rect);
    utils::get_value_or(name, task_json, "specialParams", task_info_ptr->special_params, default_ptr->special_params);
#ifdef ASST_DEBUG
    // Debug 模式下检查 roi 是否超出边界
    if (auto [x, y, w, h] = task_info_ptr->roi; x + w > WindowWidthDefault || y + h > WindowHeightDefault) {
        Log.warn(name, "roi is out of bounds");
    }
#endif
    return true;
}

std::shared_ptr<asst::MatchTaskInfo> asst::TaskData::_default_match_task_info()
{
    auto match_task_info_ptr = std::make_shared<MatchTaskInfo>();
    match_task_info_ptr->templ_names = { "__INVALID__" };
    match_task_info_ptr->templ_thresholds = { TemplThresholdDefault };

    return match_task_info_ptr;
}

std::shared_ptr<asst::OcrTaskInfo> asst::TaskData::_default_ocr_task_info()
{
    auto ocr_task_info_ptr = std::make_shared<OcrTaskInfo>();
    ocr_task_info_ptr->full_match = false;
    ocr_task_info_ptr->is_ascii = false;
    ocr_task_info_ptr->without_det = false;
    ocr_task_info_ptr->replace_full = false;

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

asst::ResultOrError<asst::TaskData::RawCompileResult> asst::TaskData::compile_raw_tasklist(
    const tasklist_t& raw_tasks, std::string_view self_name, std::function<taskptr_t(std::string_view)> get_raw,
    bool allow_duplicate)
{
    RawCompileResult ret { .task_changed = false, .symbols = {} };
    std::unordered_set<std::string_view> tasks_set; // 记录任务列表中已有的任务（内容元素与 new_tasks 基本一致）

    for (std::string_view task_expr : raw_tasks) {
        if (task_expr.empty() || (!allow_duplicate && tasks_set.contains(task_expr))) {
            ret.task_changed = true;
            continue;
        }

        TaskDataSymbolStream symbol_stream;
        if (auto opt = symbol_stream.parse(task_expr); opt) [[likely]] {
            ret.task_changed = ret.task_changed || opt.value();
        }
        else {
            return { std::nullopt, std::move(opt.what()) };
        }

        auto opt = symbol_stream.decode(
            [&](const TaskDataSymbol& symbol, const TaskDataSymbol& prefix) -> TaskDataSymbol::SymbolsOrError {
                return TaskDataSymbol::append_prefix(
                    symbol, prefix, self_name, get_raw,
                    [&](const tasklist_t& raw_or_empty) -> TaskDataSymbol::SymbolsOrError {
                        if (auto opt = compile_raw_tasklist(raw_or_empty, self_name, get_raw, allow_duplicate)) {
                            return opt.value().symbols;
                        }
                        return { std::nullopt, "" };
                    });
            },
            self_name);

        // 记号流处理
        if (!opt) {
            return { std::nullopt, std::move(opt.what()) };
        }

        for (const auto& task : *opt) {
            if (task.name().empty() || (!allow_duplicate && tasks_set.contains(task.name()))) {
                ret.task_changed = true;
                continue;
            }
            tasks_set.emplace(ret.symbols.emplace_back(task).name());
        }

        tasks_set.emplace(task_name_view(task_expr));
    }

    return ret;
}

asst::ResultOrError<asst::TaskData::CompileResult> asst::TaskData::compile_tasklist(const tasklist_t& raw_tasks,
                                                                                    std::string_view self_name,
                                                                                    bool allow_duplicate)
{
    CompileResult ret { .task_changed = false, .tasks = {} };
    std::vector<TaskDataSymbol> new_symbols;
    if (auto opt = compile_raw_tasklist(
            raw_tasks, self_name, [&](std::string_view name) { return get_raw(name); }, allow_duplicate);
        !opt) {
        return { std::nullopt, std::move(opt.what()) };
    }
    else {
        new_symbols = std::move(opt.value().symbols);
        ret.task_changed = opt.value().task_changed;
    }
    std::unordered_set<std::string_view> tasks_set;
    for (const auto& task : new_symbols) {
        std::string_view task_name;
        if (task == TaskDataSymbol::SharpSelf) {
            task_name = self_name;
            ret.task_changed = true;
        }
        else if (task.is_name()) {
            task_name = task.name();
        }
        else {
            ret.task_changed = true;
            continue;
        }
        if (task_name.empty()) {
            Log.error("Empty task name in", self_name);
            ret.task_changed = true;
            continue;
        }
        if (!allow_duplicate && tasks_set.contains(task_name)) {
            ret.task_changed = true;
            continue;
        }
        ret.tasks.emplace_back(task_name_view(task_name));
        tasks_set.emplace(task_name);
    }

    return ret;
}

#ifdef ASST_DEBUG
// 为了解决类似 beddc7c828126c678391e0b4da288db6d2c2d58a 导致的问题，加载的时候做一个语法检查
// 主要是处理是否包含未知键值的问题
bool asst::TaskData::syntax_check(std::string_view task_name, const json::value& task_json)
{
    // clang-format off
    // 以下按字典序排序
    static const std::unordered_map<AlgorithmType, std::unordered_set<std::string>> allowed_key_under_algorithm = {
        { AlgorithmType::Invalid,
          {
              "action",      "algorithm",     "baseTask",   "cache",           "exceededNext",     "fullMatch",
              "hash",        "isAscii",       "maskRange",  "maxTimes",        "next",             "ocrReplace",
              "onErrorNext", "postDelay",     "preDelay",   "rectMove",        "reduceOtherTimes", "replaceFull",
              "roi",         "specialParams", "sub",        "subErrorIgnored", "templThreshold",   "template",
              "text",        "threshold",     "withoutDet",
          } },
        { AlgorithmType::MatchTemplate,
          {
              "action",           "algorithm", "baseTask",    "cache",           "exceededNext",   "maskRange",
              "maxTimes",         "next",      "onErrorNext", "postDelay",       "preDelay",       "rectMove",
              "reduceOtherTimes", "roi",       "sub",         "subErrorIgnored", "templThreshold", "template",
              "specialParams",
          } },
        { AlgorithmType::OcrDetect,
          {
              "action",          "algorithm", "baseTask",         "cache",         "exceededNext", "fullMatch",
              "isAscii",         "maxTimes",  "next",             "ocrReplace",    "onErrorNext",  "postDelay",
              "preDelay",        "rectMove",  "reduceOtherTimes", "replaceFull",   "roi",          "sub",     
              "subErrorIgnored", "text",      "withoutDet",       "specialParams",
          } },
        { AlgorithmType::JustReturn,
          {
              "action",          "algorithm", "baseTask", "exceededNext",     "maxTimes",      "next",
              "onErrorNext",     "postDelay", "preDelay", "reduceOtherTimes", "specialParams", "sub",
              "subErrorIgnored",
          } },
        { AlgorithmType::Hash,
          {
              "action",    "algorithm",        "baseTask", "cache",         "exceededNext", "hash",
              "maskRange", "maxTimes",         "next",     "onErrorNext",   "postDelay",    "preDelay",
              "rectMove",  "reduceOtherTimes", "roi",      "specialParams", "sub",          "subErrorIgnored",
              "threshold",
          } },
    };
    // clang-format on

    static const std::unordered_map<ProcessTaskAction, std::unordered_set<std::string>> allowed_key_under_action = {
        { ProcessTaskAction::ClickRect, { "specificRect" } },
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

    if (!task_json.is_object()) {
        Log.error(task_name, "is not a json object.");
        return false;
    }

    bool validity = true;
    auto task_ptr = get(task_name);
    if (task_ptr == nullptr) {
        Log.error("TaskData::syntax_check | Task", task_name, "has not been generated.");
        return false;
    }

    // 获取 algorithm
    auto algorithm = task_ptr->algorithm;
    if (algorithm == AlgorithmType::Invalid) [[unlikely]] {
        Log.error(task_name, "has unknown algorithm.");
        validity = false;
    }

    // 获取 action
    auto action = task_ptr->action;
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
