#include "TaskDataSymbolStream.h"

asst::ResultOrError<bool> asst::TaskDataSymbolStream::parse(const std::string& task_expr)
{
    bool task_changed = false;
    auto emplace_symbol_if_not_empty = [&](const auto l, const auto r) {
        if (l < r) {
            std::string symbol(l, r);
            auto symbol_type = Symbol::type(symbol);
            if (Symbol::is_sharp_type(symbol_type)) {
                if (!m_symbolstream.empty() && m_symbolstream.back() == Symbol::Sharp) {
                    m_symbolstream.pop_back();
                    if (!m_symbolstream.empty() &&
                        (m_symbolstream.back().is_name() || m_symbolstream.back().is_sharp_type() ||
                         m_symbolstream.back() == Symbol::RParen)) {
                        m_symbolstream.emplace_back(Symbol::At);
                    }
                    m_symbolstream.emplace_back(symbol_type);
                }
                else {
                    m_symbolstream.emplace_back(symbol);
                }
            }
            else if (symbol_type != Symbol::Name) {
                m_symbolstream.emplace_back(symbol_type);
            }
            else {
                m_symbolstream.emplace_back(symbol);
            }
        }
    };

    // 记号流分析
    auto y_begin = task_expr.cbegin();
    for (auto p = task_expr.cbegin(); p != task_expr.cend(); ++p) {
        switch (*p) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            task_changed = true;
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
        case '@': {
            emplace_symbol_if_not_empty(y_begin, p);
            y_begin = p + 1;
            auto symbol = TaskDataSymbol::type(std::string { std::addressof(*p), 1 });
            if (symbol == TaskDataSymbol::Name) [[unlikely]] {
                // should not happen
                return { std::nullopt,
                         std::string("Error when decode symbol ") + *p + " at " +
                             std::to_string(p - task_expr.cbegin()) + " in " += task_expr };
            }
            m_symbolstream.emplace_back(symbol);
            break;
        }
        default:
            break;
        }
    }
    emplace_symbol_if_not_empty(y_begin, task_expr.cend());
    m_symbolstream.emplace_back(TaskDataSymbol::End); // 结束符

    return task_changed;
}

asst::TaskDataSymbolStream::SymbolsOrError
    asst::TaskDataSymbolStream::decode(AppendPrefixFunc append_prefix, const std::string& self_name) const
{
    /*
    $name         = 至少一位的任务名
    $numbers      = 至少一位的数字

    $subtask_type = # sub
                    | # next
                    | # on_error_next
                    | # exceeded_next
                    | # reduce_other_times

    $sharp_type   = $subtask_type
                    | # self
                    | # back
                    | # none

    $subtask      = $subtask_type ( $tasks )

    $lambda_task  = { $subtask , ... }           // 暂未实现
                    | { $tasks }                 // 暂未实现；不写 subtask_type 默认为 sub

    $parens       = $sharp_type
                    | $name
                    | $name $lambda_task         // 暂未实现
                    | ( $tasks )

    $top_tasks    = $top_tasks @ $parens
                    | $parens

    $mul_tasks    = $top_tasks * $numbers
                    | $top_tasks

    $tasks        = $mul_tasks + $tasks
                    | $mul_tasks ^ $tasks         // 删除 $mul_tasks 中所有在 $tasks 中的任务
                    | $mul_tasks
    */
    using decode_func_t = std::function<SymbolsOrError()>;

    auto cur = m_symbolstream.cbegin();
    decode_func_t decode_name_or_vtask, decode_tasks, decode_multasks, decode_vtasks, decode_parens;
    decode_name_or_vtask = [&]() -> SymbolsOrError {
        if (cur->is_name() || cur->is_sharp_type()) {
            return Symbols { *(cur++) };
        }
        return { std::nullopt, "expect name or sharp type, got " + cur->name() };
    };
    decode_tasks = [&]() -> SymbolsOrError {
        Symbols x = {}, y = {};
        if (auto opt = decode_multasks(); !opt) {
            return opt;
        }
        else {
            x = *opt;
        }
        while (*cur == Symbol::Add || *cur == Symbol::Sub) {
            Symbol op = *(cur++);
            if (auto opt = decode_multasks(); !opt) {
                return opt;
            }
            else {
                y = *opt;
            }
            if (op == Symbol::Add) {
                // x = x + y
                std::ranges::move(y, std::back_inserter(x));
            }
            else {
                // x = x - y
                // self_name + #self ^ #self = #none
                // self_name + #self ^ self_name = #none
                if (std::ranges::any_of(y, [&](const auto& sy) { return sy == Symbol::SharpSelf; })) {
                    std::erase(x, self_name);
                }
                if (std::ranges::any_of(y, [&](const auto& sy) { return sy == self_name; })) {
                    std::erase(x, Symbol::SharpSelf);
                }
                std::erase_if(x, [&](const auto& sx) {
                    return std::ranges::any_of(y, [&](const auto& sy) { return sx == sy; });
                });
            }
        }
        return x;
    };
    decode_multasks = [&]() -> SymbolsOrError {
        Symbols x = {}, y = {};
        if (auto opt = decode_vtasks(); !opt) {
            return opt;
        }
        else {
            x = *opt;
        }
        while (*cur == Symbol::Mul) {
            Symbol op = *(cur++);
            if (auto opt = decode_name_or_vtask(); !opt) {
                return opt;
            }
            else {
                y = *opt;
            }
            if (y.size() != 1) {
                return { std::nullopt, "too many y" };
            }
            int times = 0;
            if (const auto s = y.front(); !s.is_name() || !utils::chars_to_number<int, true>(s.name(), times)) {
                return { std::nullopt, "y is not number" };
            }
            if (times < 0) {
                return { std::nullopt, "y is negative" };
            }
            if (times == 0) {
                x = {};
                Log.warn("y:", times, "is zero");
                continue;
            }
            if (times == 1) {
                continue;
            }
            Symbols x_copy = x;
            for (int i = 0; i < times - 1; ++i) {
                std::ranges::copy(x_copy, std::back_inserter(x));
            }
        }
        return x;
    };
    decode_vtasks = [&]() -> SymbolsOrError {
        Symbols x = {}, y = {};
        if (auto opt = decode_parens(); !opt) {
            return opt;
        }
        else {
            x = *opt;
        }
        while (*cur == Symbol::At) {
            Symbol op = *(cur++);
            if (auto opt = decode_parens(); !opt) {
                return opt;
            }
            else {
                y = *opt;
            }
            // (A+B)@(C+D) = A@C + A@D + B@C + B@D
            Symbols x_copy = x;
            x.clear();
            for (const auto& sx : x_copy) {
                for (const auto& sy : y) {
                    auto opt = append_prefix(sy, sx);
                    if (!opt) {
                        return { std::nullopt,
                                 "decode_vtasks: failed while " + sx.name() + " @ " + sy.name() + ", " + opt.error() };
                    }
                    std::ranges::copy(*opt, std::back_inserter(x));
                }
            }
        }
        return x;
    };
    decode_parens = [&]() -> SymbolsOrError {
        if (*cur == Symbol::LParen) {
            ++cur;
            Symbols x = {};
            if (auto opt = decode_tasks(); !opt) {
                return opt;
            }
            else {
                x = *opt;
            }
            if (*cur != Symbol::RParen) [[unlikely]] {
                return { std::nullopt, "decode_parens: expected ')', got " + cur->name() };
            }
            ++cur;
            return x;
        }
        if (cur->is_name() || cur->is_sharp_type()) {
            return decode_name_or_vtask();
        }
        return { std::nullopt, "decode_parens: unexpected symbol " + cur->name() };
    };

    auto opt = decode_tasks();
    if (!opt) {
        return opt;
    }
    if (cur != m_symbolstream.cend() && *cur != TaskDataSymbol::End) {
        return { std::nullopt, "decode: did not reach end, got " + cur->name() };
    }

    return opt;
}
