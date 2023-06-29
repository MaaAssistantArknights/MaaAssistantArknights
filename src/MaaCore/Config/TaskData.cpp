#include "TaskData.h"

#include <algorithm>
#include <meojson/json.hpp>

#ifdef ASST_DEBUG
#include <queue>
#endif

#include "Common/AsstTypes.h"
#include "GeneralConfig.h"
#include "TemplResource.h"
#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

const std::unordered_set<std::string>& asst::TaskData::get_templ_required() const noexcept
{
    return m_templ_required;
}
std::shared_ptr<asst::TaskInfo> asst::TaskData::get_raw(std::string_view name) const
{
    // 普通 task 或已经生成过的 `@` 型 task
    if (auto it = m_raw_all_tasks_info.find(name); it != m_raw_all_tasks_info.cend()) [[likely]] {
        return it->second;
    }

    size_t at_pos = name.find('@');
    if (at_pos == std::string_view::npos) [[unlikely]] {
        return nullptr;
    }

    // `@` 前面的字符长度
    size_t name_len = at_pos;
    auto base_task_iter = get_raw(name.substr(name_len + 1));
    if (base_task_iter == nullptr) [[unlikely]] {
        return nullptr;
    }

    std::string_view derived_task_name = name.substr(0, name_len);
    return _generate_task_info(base_task_iter, derived_task_name);
}

std::shared_ptr<asst::TaskInfo> asst::TaskData::get(std::string_view name)
{
    // 普通 task 或已经生成过的 `@` 型 task
    if (auto it = m_all_tasks_info.find(name); it != m_all_tasks_info.cend()) [[likely]] {
        return it->second;
    }

    return expand_task(name, get_raw(name)).value_or(nullptr);
}

bool asst::TaskData::parse(const json::value& json)
{
    LogTraceFunction;

    const auto& json_obj = json.as_object();

    {
        enum TaskStatus
        {
            NotToBeGenerate = 0, // 已经显式生成 或 不是待显式生成 的资源
            ToBeGenerate,        // 待生成 的资源
            Generating,          // 正在生成 的资源
            NotExists,           // 不存在的资源
        };
        std::unordered_map<std::string_view, TaskStatus> task_status;
        for (const std::string& name : json_obj | views::keys) {
            task_status[task_name_view(name)] = ToBeGenerate;
        }

        auto generate_task_and_its_base = [&](const std::string& name) -> bool {
            auto generate_task = [&](const std::string& name, std::string_view prefix, taskptr_t base_ptr,
                                     const json::value& task_json) {
                auto task_info_ptr = generate_task_info(name, task_json, base_ptr, prefix);
                if (task_info_ptr == nullptr) {
                    return false;
                }
                task_status[task_name_view(name)] = NotToBeGenerate;
                insert_or_assign_raw_task(name, task_info_ptr);
                return true;
            };
            std::function<bool(const std::string&, bool)> generate_fun;
            generate_fun = [&](const std::string& name, bool must_true) -> bool {
                switch (task_status[task_name_view(name)]) {
                case NotToBeGenerate:
                    // 已经显式生成 或 曾经显式生成（外服隐式引用国服资源）
                    if (m_raw_all_tasks_info.contains(name)) {
                        return true;
                    }

                    // 隐式生成的资源
                    if (size_t p = name.find('@'); p != std::string::npos) {
                        return generate_fun(name.substr(p + 1), must_true);
                    }

                    task_status[name] = NotExists;
                    [[fallthrough]];
                case NotExists:
                    if (must_true) {
                        // 必须有名字为 name 的资源
                        Log.error("Unknown task:", name);
                    }
                    // 不一定必须有名字为 name 的资源，例如 Roguelike@Abandon 不必有 Abandon.
                    return false;
                case ToBeGenerate: {
                    task_status[name] = Generating;
                    const json::value& task_json = json_obj.at(name);

                    if (auto opt = task_json.find<std::string>("baseTask")) {
                        // BaseTask
                        std::string base = opt.value();
                        if (base == "#none") {
                            // `"baseTask": "#none"` 表示不使用已生成的同名任务
                        }
                        else if (base.empty()) {
                            Log.warn("Use `\"baseTask\": \"#none\"` instead of `\"baseTask\": \"\"` in Task", name);
                        }
                        else {
                            return generate_fun(base, must_true) && generate_task(name, "", get_raw(base), task_json);
                        }
                    }
                    else if (m_raw_all_tasks_info.contains(name)) {
                        // 已生成（外服覆写国服资源）
                        return generate_task(name, "", get_raw(name), task_json);
                    }

                    // TemplateTask
                    if (size_t p = name.find('@'); p != std::string::npos) {
                        if (std::string base = name.substr(p + 1); generate_fun(base, false)) {
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
            };
            return generate_fun(name, true);
        };

        for (const std::string& name : json_obj | views::keys) {
            generate_task_and_its_base(name);
        }

        // 延迟展开，等到一个任务被第一次 get 的时候才展开
        // debug 时为了做语法检查，会 *提前* 展开
        /*
        // 展开 # 型任务
        for (const auto& [name, old_task] : m_raw_all_tasks_info) {
            expand_task(name, old_task);
        }
        */
    }

#ifdef ASST_DEBUG
    {
        bool validity = true;

        std::queue<std::string_view> task_queue;
        std::unordered_set<std::string_view> checking_task_set;
        for (const auto& [name, task_json] : json_obj) [[likely]] {
            // 语法检查
            validity &= syntax_check(name, task_json);
            task_queue.push(name);
            checking_task_set.insert(name);
        }

        const size_t MAX_CHECKING_SIZE = 3000;
        while (!task_queue.empty() || checking_task_set.size() > MAX_CHECKING_SIZE) {
            std::string_view name = task_queue.front();
            task_queue.pop();
            auto task = get(name); // 这里会提前展开任务
            auto check_tasklist = [&](const tasklist_t& task_list, std::string_view list_type,
                                      bool enable_justreturn_check = false) {
                std::unordered_set<std::string_view> tasks_set {};
                std::string justreturn_task_name = "";
                for (const std::string& task_name : task_list) {
                    if (tasks_set.contains(task_name)) [[unlikely]] {
                        continue;
                    }
                    if (!checking_task_set.contains(task_name)) {
                        task_queue.emplace(task_name_view(task_name));
                        checking_task_set.emplace(task_name_view(task_name));
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
                    }
                    else if (ptr->algorithm == AlgorithmType::JustReturn) {
                        justreturn_task_name = ptr->name;
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
        if (!validity) return false;
    }
#endif
    return true;
}

// new_tasks 是目的任务列表
// raw_tasks 是源任务表达式列表
// task_name 是任务名
// task_changed 记录 raw_tasks 在转换为 new_tasks 时是否发生变化
// multi 表示是否允许重复
bool asst::TaskData::explain_tasks(tasklist_t& new_tasks, const tasklist_t& raw_tasks, std::string_view task_name,
                                   bool& task_changed, bool multi)
{
    std::unordered_set<std::string_view> tasks_set; // 记录任务列表中已有的任务（内容元素与 new_tasks 基本一致）

    using symbl_t = size_t;
    [[maybe_unused]] constexpr symbl_t symbl_end = 0;
    [[maybe_unused]] constexpr symbl_t symbl_lambda_task_sep = 1;
    [[maybe_unused]] constexpr symbl_t symbl_lparen = 2;
    [[maybe_unused]] constexpr symbl_t symbl_rparen = 3;
    [[maybe_unused]] constexpr symbl_t symbl_lbrace = 4;
    [[maybe_unused]] constexpr symbl_t symbl_rbrace = 5;
    [[maybe_unused]] constexpr symbl_t symbl_at = 6;
    [[maybe_unused]] constexpr symbl_t symbl_sharp = 7;
    [[maybe_unused]] constexpr symbl_t symbl_mul = 8;
    [[maybe_unused]] constexpr symbl_t symbl_add = 9;
    [[maybe_unused]] constexpr symbl_t symbl_sub = 10;

    [[maybe_unused]] constexpr symbl_t symbl_name_start = 11;

    [[maybe_unused]] constexpr symbl_t symbl_name_sub = 11;
    [[maybe_unused]] constexpr symbl_t symbl_name_next = 12;
    [[maybe_unused]] constexpr symbl_t symbl_name_on_error_next = 13;
    [[maybe_unused]] constexpr symbl_t symbl_name_exceeded_next = 14;
    [[maybe_unused]] constexpr symbl_t symbl_name_reduce_other_times = 15;
    [[maybe_unused]] constexpr symbl_t symbl_name_self = 16;
    [[maybe_unused]] constexpr symbl_t symbl_name_back = 17;
    [[maybe_unused]] constexpr symbl_t symbl_name_none = 18;

    static const std::vector<std::string> symbl_table = {
        "__END__",            // 0
        ",",                  // 1
        "(",                  // 2
        ")",                  // 3
        "{",                  // 4
        "}",                  // 5
        "@",                  // 6
        "#",                  // 7
        "*",                  // 8
        "+",                  // 9
        "^",                  // 10
        "sub",                // 11
        "next",               // 12
        "on_error_next",      // 13
        "exceeded_next",      // 14
        "reduce_other_times", // 15
        "self",               // 16
        "back",               // 17
        "none",               // 18
    };

    [[maybe_unused]] auto is_symbl_name = [&](symbl_t x) { return x >= symbl_name_start; };
    auto is_symbl_subtask_type = [&](symbl_t x) {
        switch (x) {
        case symbl_name_sub:
        case symbl_name_next:
        case symbl_name_on_error_next:
        case symbl_name_exceeded_next:
        case symbl_name_reduce_other_times:
            return true;
        default:
            return false;
        }
    };
    [[maybe_unused]] auto is_symbl_sharp_type = [&](symbl_t x) {
        switch (x) {
        case symbl_name_self:
        case symbl_name_back:
        case symbl_name_none:
            return true;
        default:
            return is_symbl_subtask_type(x);
        }
    };

    // perform_op 的结果不保证符合参数 multi 的要求
    auto perform_op = [&](std::string_view task_expr, symbl_t op, tasklistptr_t x,
                          tasklistptr_t y) -> std::optional<asst::TaskData::tasklistptr_t> {
        auto ret = std::make_shared<tasklist_t>();

        switch (op) {
        case symbl_add:
            ranges::copy(*x, std::back_inserter(*ret));
            ranges::copy(*y, std::back_inserter(*ret));
            break;
        case symbl_sub:
            for (std::string_view sx : *x) {
                bool flag = true;
                for (std::string_view sy : *y) {
                    if (sx == sy) {
                        flag = false;
                        break;
                    }
                }
                if (flag) ret->emplace_back(sx);
            }
            break;
        case symbl_mul: {
            if (y->size() != 1) {
                Log.error("There is more than one y:", *y, "while perform op", symbl_table[op], "in", task_expr,
                          "of task:", task_name);
                return std::nullopt;
            }
            int times = 0;
            try {
                times = std::stoi(y->front());
            }
            catch (...) {
                Log.error("y:", y->front(), "is not number while perform op", symbl_table[op], "in", task_expr,
                          "of task:", task_name);
                return std::nullopt;
            }
            for (int i = 0; i < times; ++i) {
                ranges::copy(*x, std::back_inserter(*ret));
            }
            break;
        }
        case symbl_at: {
            if (y->empty()) { // A@#none = A
                ranges::copy(*x, std::back_inserter(*ret));
            }
            else if (x->empty()) { // #none@A = A
                ranges::copy(*y, std::back_inserter(*ret));
            }
            else { // (A+B)@(C+D) = A@C + A@D + B@C + B@D
                ranges::for_each(
                    *x, [&](std::string_view s) { ranges::copy(append_prefix(*y, s), std::back_inserter(*ret)); });
            }
            break;
        }
        case symbl_sharp: {
            if (y->empty()) {
                Log.error("There is no sharp_type while perform op", symbl_table[op], "in", task_expr,
                          "of task:", task_name);
                return std::nullopt;
            }
            for (std::string_view type : *y) {
                if (!x || x->empty()) { // unary
                    x = std::make_shared<tasklist_t>(tasklist_t { "" });
                }
                for (const auto& task : *x) {
                    if (type == "self") {
                        ret->emplace_back(task_name);
                        continue;
                    }
                    if (type == "none") {
                        continue;
                    }
                    if (type == "back") {
                        // "A#back" === "A", "B@A#back" === "B@A", "#back" === null
                        if (!task.empty()) ret->emplace_back(task);
                        continue;
                    }
                    taskptr_t other_task_info_ptr = task.empty() ? default_task_info_ptr : get_raw(task);

#define ASST_TASKDATA_PERFORM_OP_IF_BRANCH(t, m)                                                                      \
    else if (type == #t)                                                                                              \
    {                                                                                                                 \
        bool task_changed = false;                                                                                    \
        if (!explain_tasks(*ret, other_task_info_ptr->t, task_name, task_changed, m)) {                               \
            Log.error("Failed to explain task", task + "->" #t, "while perform op", symbl_table[op], "in", task_expr, \
                      "of task:", task_name);                                                                         \
            return std::nullopt;                                                                                      \
        }                                                                                                             \
    }
                    if (other_task_info_ptr == nullptr) [[unlikely]] {
                        Log.error("Task", task, "not found while perform op", symbl_table[op], "in", task_expr,
                                  "of task:", task_name);
                        return std::nullopt;
                    }
                    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(next, false)
                    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(sub, true)
                    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(on_error_next, false)
                    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(exceeded_next, false)
                    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(reduce_other_times, true)
                    else [[unlikely]]
                    {
                        Log.error("Unknown symbol", type, "while perform op", symbl_table[op], "in", task_expr,
                                  "of task:", task_name);
                        return std::nullopt;
                    }
#undef ASST_TASKDATA_PERFORM_OP_IF_BRANCH
                }
            }

            break;
        }
        default:
            Log.error("Unknown op", symbl_table[op], "in", task_expr, "of task:", task_name);
            return std::nullopt;
        }
        return ret;
    };

    for (std::string_view task_expr : raw_tasks) {
        if (task_expr.empty() || (!multi && tasks_set.contains(task_expr))) {
            task_changed = true;
            continue;
        }

        std::vector<std::string> symbols_table = symbl_table;

        std::unordered_map<std::string, symbl_t> symbols_id = {};
        for (symbl_t i = 0; i != symbols_table.size(); ++i) {
            symbols_id.emplace(symbols_table[i], i);
        }

        std::vector<symbl_t> symbol_stream;
        auto emplace_symbol_if_not_empty = [&](const auto l, const auto r) {
            if (l < r) {
                auto symbol = std::string(l, r);
                if (!symbols_id.contains(symbol)) {
                    symbl_t id = symbols_table.size();
                    symbols_id.emplace(symbol, id);
                    symbols_table.emplace_back(symbol);
                    symbol_stream.emplace_back(id);
                }
                else {
                    symbol_stream.emplace_back(symbols_id[symbol]);
                }
            }
        };

        // 记号流分析
        auto y_begin = task_expr.begin();
        for (auto p = task_expr.begin(); p != task_expr.end(); ++p) {
            switch (*p) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                emplace_symbol_if_not_empty(y_begin, p);
                y_begin = p + 1;
                break;
            case ',':
            case '(':
            case ')':
            case '{':
            case '}':
            case '#':
            case '*':
            case '+':
            case '^':
                task_changed = true;
                [[fallthrough]];
            case '@':
                emplace_symbol_if_not_empty(y_begin, p);
                y_begin = p + 1;
                symbol_stream.emplace_back(symbols_id[std::string(1, *p)]);
                break;
            default:
                break;
            }
        }
        emplace_symbol_if_not_empty(y_begin, task_expr.end());
        symbol_stream.emplace_back(symbl_end); // 结束符
        // Log.debug(symbol_stream | views::transform([&](symbl_t id) { return symbols_table[id]; }));
        // Log.debug(symbol_stream);

        /*
        $name         = 至少一位的任务名
        $numbers      = 至少一位的数字

        $subtask_type = sub
                      | next
                      | on_error_next
                      | exceeded_next
                      | reduce_other_times

        $sharp_type   = $subtask_type
                      | self
                      | back
                      | none

        $subtask      = $subtask_type ( $tasks )

        $lambda_task  = { $subtask , ... }         // 推迟实现
                      | { $tasks }                 // 推迟实现；不写 subtask_type 默认为 sub

        $parens       = # $sharp_type
                      | $name
                      | $name $lambda_task         // 推迟实现
                      | ( $tasks )

        $top_tasks    = $top_tasks @ $parens
                      | $top_tasks # $sharp_type
                      | $parens

        $mul_tasks    = $top_tasks * $numbers
                      | $top_tasks

        $tasks        = $mul_tasks + $tasks
                      | $mul_tasks ^ $tasks         // 删除 $mul_tasks 中所有在 $tasks 中的任务
                      | $mul_tasks
        */
        // 记号流处理
        auto cur = symbol_stream.cbegin();
        using decode_func_t = std::function<std::optional<tasklistptr_t>()>;
        decode_func_t decode_name, decode_tasks, decode_multasks, decode_vtasks, decode_parens;
        decode_name = [&]() -> std::optional<tasklistptr_t> {
            if (*cur >= symbl_name_start) {
                return std::make_shared<tasklist_t>(tasklist_t { symbols_table[*(cur++)] });
            }
            return std::nullopt;
        };
        decode_tasks = [&]() -> std::optional<tasklistptr_t> {
            auto l = decode_multasks();
            if (!l) return std::nullopt;
            while (*cur == symbl_add || *cur == symbl_sub) {
                symbl_t op = *(cur++);
                auto r = decode_multasks();
                if (!r) return std::nullopt;
                l = perform_op(task_expr, op, *l, *r);
                if (!l) return std::nullopt;
            }
            return l;
        };
        decode_multasks = [&]() -> std::optional<tasklistptr_t> {
            auto l = decode_vtasks();
            if (!l) return std::nullopt;
            while (*cur == symbl_mul) {
                symbl_t op = *(cur++);
                auto r = decode_name();
                if (!r) return std::nullopt;
                l = perform_op(task_expr, op, *l, *r);
                if (!l) return std::nullopt;
            }
            return l;
        };
        decode_vtasks = [&]() -> std::optional<tasklistptr_t> {
            auto l = decode_parens();
            if (!l) return std::nullopt;
            while (*cur == symbl_at || *cur == symbl_sharp) {
                symbl_t op = *(cur++);
                auto r = decode_parens();
                if (!r) return std::nullopt;
                l = perform_op(task_expr, op, *l, *r);
                if (!l) return std::nullopt;
            }
            return l;
        };
        decode_parens = [&]() -> std::optional<tasklistptr_t> {
            if (*cur == symbl_lparen) {
                ++cur;
                auto l = decode_tasks();
                if (!l) return std::nullopt;
                if (*cur != symbl_rparen) [[unlikely]] {
                    Log.error("Invalid symbol", *cur, "at", cur - symbol_stream.cbegin(), "in", symbol_stream,
                              "(should be ')')");
                    return std::nullopt;
                }
                ++cur;
                return l;
            }
            if (*cur == symbl_sharp) {
                ++cur;
                auto r = decode_name();
                if (!r) return std::nullopt;
                return perform_op(task_expr, symbl_sharp, nullptr, *r);
            }
            if (*cur >= symbl_name_start) {
                return decode_name();
            }
            Log.error("Invalid symbol", *cur, "at", cur - symbol_stream.cbegin(), "in", symbol_stream);
            return std::nullopt;
        };
        auto opt = decode_tasks();
        if (!opt || (*cur != symbl_end && cur != symbol_stream.cend())) {
            return false;
        }

        for (const auto& task : **opt) {
            if (task.empty() || (!multi && tasks_set.contains(task))) {
                task_changed = true;
                continue;
            }
            new_tasks.emplace_back(task);
            tasks_set.emplace(task_name_view(task));
        }

        tasks_set.emplace(task_name_view(task_expr));
    }

    return true;
}

std::optional<asst::TaskData::taskptr_t> asst::TaskData::expand_task(std::string_view name, taskptr_t old_task)
{
    if (old_task == nullptr) [[unlikely]] {
        return std::nullopt;
    }
    auto task_info = _generate_task_info(old_task); // 复制一份
    bool task_changed = false;                      // 任务列表是否发生变化
#define ASST_TASKDATA_EXPEND_TASK_IF_BRANCH(type, m)                                           \
    task_info->type.clear();                                                                   \
    if (!explain_tasks(task_info->type, old_task->type, name, task_changed, m)) [[unlikely]] { \
        Log.error("Generate task_list", std::string(name) + "->" #type, "failed.");            \
        return std::nullopt;                                                                   \
    }
    ASST_TASKDATA_EXPEND_TASK_IF_BRANCH(next, false);
    ASST_TASKDATA_EXPEND_TASK_IF_BRANCH(sub, true);
    ASST_TASKDATA_EXPEND_TASK_IF_BRANCH(exceeded_next, false);
    ASST_TASKDATA_EXPEND_TASK_IF_BRANCH(on_error_next, false);
    ASST_TASKDATA_EXPEND_TASK_IF_BRANCH(reduce_other_times, true);
#undef ASST_TASKDATA_EXPEND_TASK_IF_BRANCH

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
    ocr_task_info_ptr->without_det = task_json.get("withoutDet", default_ptr->without_det);
    ocr_task_info_ptr->replace_full = task_json.get("replaceFull", default_ptr->replace_full);
    if (auto opt = task_json.find<json::array>("ocrReplace")) {
        for (const json::value& rep : opt.value()) {
            ocr_task_info_ptr->replace_map.emplace_back(std::make_pair(rep[0].as_string(), rep[1].as_string()));
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

#ifdef ASST_DEBUG
// 为了解决类似 beddc7c828126c678391e0b4da288db6d2c2d58a 导致的问题，加载的时候做一个语法检查
// 主要是处理是否包含未知键值的问题
bool asst::TaskData::syntax_check(const std::string& task_name, const json::value& task_json)
{
    // clang-format off
    // 以下按字典序排序
    static const std::unordered_map<AlgorithmType, std::unordered_set<std::string>> allowed_key_under_algorithm = {
        { AlgorithmType::Invalid,
          {
              "action",        "algorithm", "baseTask",        "cache",          "exceededNext",     "fullMatch",
              "hash",          "isAscii",   "maskRange",       "maxTimes",       "next",             "ocrReplace",
              "onErrorNext",   "postDelay", "preDelay",        "rectMove",       "reduceOtherTimes", "replaceFull", "roi",
              "specialParams", "sub",       "subErrorIgnored", "templThreshold", "template",         "text",
              "threshold",     "withoutDet",
          } },
        { AlgorithmType::MatchTemplate,
          {
              "action",           "algorithm", "baseTask",    "cache",           "exceededNext",   "maskRange",
              "maxTimes",         "next",      "onErrorNext", "postDelay",       "preDelay",       "rectMove",
              "reduceOtherTimes", "roi",       "sub",         "subErrorIgnored", "templThreshold", "template",
              "specialParams"
          } },
        { AlgorithmType::OcrDetect,
          {
              "action",      "algorithm", "baseTask",        "cache",    "exceededNext",
              "fullMatch",   "isAscii",   "maxTimes",        "next",     "ocrReplace",
              "onErrorNext", "postDelay", "preDelay",        "rectMove", "reduceOtherTimes", "replaceFull",
              "roi",         "sub",       "subErrorIgnored", "text",     "withoutDet",
              "specialParams"
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

    if (!task_json.is_object()) {
        Log.error(task_name, "is not a json object.");
        return false;
    }

    bool validity = true;
    if (!m_raw_all_tasks_info.contains(task_name)) {
        Log.error("TaskData::syntax_check | Task", task_name, "has not been generated.");
        return false;
    }

    // 获取 algorithm
    auto algorithm = m_raw_all_tasks_info[task_name]->algorithm;
    if (algorithm == AlgorithmType::Invalid) [[unlikely]] {
        Log.error(task_name, "has unknown algorithm.");
        validity = false;
    }

    // 获取 action
    auto action = m_raw_all_tasks_info[task_name]->action;
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
