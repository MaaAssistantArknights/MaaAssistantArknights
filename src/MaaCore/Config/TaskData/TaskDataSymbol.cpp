#include "TaskDataSymbol.h"

asst::TaskDataSymbol::SymbolsOrError asst::TaskDataSymbol::append_prefix(
    const TaskDataSymbol& symbol,
    const TaskDataSymbol& prefix,
    std::string_view self_name,
    std::function<TaskDerivedConstPtr(std::string_view)> get_raw,
    std::function<SymbolsOrError(const TaskList&)> compile_tasklist)
{
    // 注意：A@#self 是 A 而不是 A@self_name, #self@A 是 self_name@A 而不是 A
    std::string_view prefix_name;
    if (prefix == SharpSelf) {
        prefix_name = self_name;
    }
    else if (prefix.is_name()) {
        prefix_name = prefix.name();
    }
    else [[unlikely]] {
        return { std::nullopt, "prefix " + prefix.name() + " is not name or self" };
    }
    if (prefix_name.empty()) {
        return std::vector { symbol };
    }
    if (symbol == SharpNone) {
        return std::vector { symbol };
    }
    if (symbol == SharpSelf) {
        return std::vector { symbol };
    }
    if (symbol.is_name()) {
        if (symbol.name().empty()) {
            return { std::nullopt, "name is empty" };
        }
        return std::vector { TaskDataSymbol(std::string(prefix_name) + '@' + symbol.name()) };
    }
    if (symbol == SharpBack) {
        return std::vector { TaskDataSymbol(prefix_name) };
    }
    auto other_task_info_ptr = get_raw(prefix_name);
#define ASST_TASKDATA_PERFORM_OP_IF_BRANCH(t, s, m)                \
    if (symbol == s) {                                             \
        if (auto opt = compile_tasklist(other_task_info_ptr->t)) { \
            return opt.value();                                    \
        }                                                          \
        return { std::nullopt, "failed to compile tasklist" };     \
    }
    if (other_task_info_ptr == nullptr) [[unlikely]] {
        return { std::nullopt, "task not found" };
    }
    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(next, SharpNext, false)
    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(sub, SharpSub, true)
    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(on_error_next, SharpOnErrorNext, false)
    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(exceeded_next, SharpExceededNext, false)
    ASST_TASKDATA_PERFORM_OP_IF_BRANCH(reduce_other_times, SharpReduceOtherTimes, true)
#undef ASST_TASKDATA_PERFORM_OP_IF_BRANCH
    return { std::nullopt, "unknown symbol" };
}
