#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "Common/AsstTypes.h"
#include "TaskDataTypes.h"
#include "Utils/Logger.hpp"

namespace asst
{
class TaskDataSymbol final
{
public:
    using Symbols = std::vector<TaskDataSymbol>;
    using SymbolsOrError = ResultOrError<Symbols>;

    enum Type
    {
        End,
        LambdaTaskSep,
        LParen,
        RParen,
        LBrace,
        RBrace,
        At,
        Sharp,
        Mul,
        Add,
        Sub,
        SharpSub,
        SharpNext,
        SharpOnErrorNext,
        SharpExceededNext,
        SharpReduceOtherTimes,
        SharpSelf,
        SharpBack,
        SharpNone,
        Name,
    };

    static const inline std::unordered_map<std::string_view, Type> symbol_repr_to_type = {
        { "__END__", End },
        { ",", LambdaTaskSep },
        { "(", LParen },
        { ")", RParen },
        { "{", LBrace },
        { "}", RBrace },
        { "@", At },
        { "#", Sharp },
        { "*", Mul },
        { "+", Add },
        { "^", Sub },
        { "sub", SharpSub },
        { "next", SharpNext },
        { "on_error_next", SharpOnErrorNext },
        { "exceeded_next", SharpExceededNext },
        { "reduce_other_times", SharpReduceOtherTimes },
        { "self", SharpSelf },
        { "back", SharpBack },
        { "none", SharpNone },
        { "__name__", Name },
    };
    static const inline std::unordered_map<Type, std::string> symbol_type_to_repr = {
        { End, "__END__" },
        { LambdaTaskSep, "," },
        { LParen, "(" },
        { RParen, ")" },
        { LBrace, "{" },
        { RBrace, "}" },
        { At, "@" },
        { Sharp, "#" },
        { Mul, "*" },
        { Add, "+" },
        { Sub, "^" },
        { SharpSub, "sub" },
        { SharpNext, "next" },
        { SharpOnErrorNext, "on_error_next" },
        { SharpExceededNext, "exceeded_next" },
        { SharpReduceOtherTimes, "reduce_other_times" },
        { SharpSelf, "self" },
        { SharpBack, "back" },
        { SharpNone, "none" },
        { Name, "__name__" },
    };

    Type m_symbol;
    std::string m_name;

public:
    bool operator==(Type other) const noexcept { return m_symbol == other; }

    bool operator==(std::string_view other) const noexcept { return m_symbol == Name && m_name == other; }

    bool operator==(const TaskDataSymbol& other) const noexcept
    {
        return m_symbol == other.m_symbol && (m_symbol != Name || m_name == other.m_name);
    }

    TaskDataSymbol(Type symbol) :
        m_symbol(symbol)
    {
    }

    template <typename StringT>
    requires(std::constructible_from<std::string, StringT>)
    TaskDataSymbol(StringT&& name) :
        m_symbol(Name),
        m_name(std::forward<StringT>(name))
    {
    }

    static bool is_sharp_type(Type type) noexcept
    {
        static std::unordered_set<Type> sharp_types = {
            SharpSub,  SharpNext, SharpOnErrorNext, SharpExceededNext, SharpReduceOtherTimes,
            SharpSelf, SharpBack, SharpNone,
        };
        return sharp_types.contains(type);
    }

    static const std::string& repr(Type type)
    {
        const auto pos = symbol_type_to_repr.find(type);
        return pos == symbol_type_to_repr.end() ? symbol_type_to_repr.at(Name) : pos->second;
    }

    static Type type(std::string_view repr)
    {
        const auto pos = symbol_repr_to_type.find(repr);
        return pos == symbol_repr_to_type.end() ? Name : pos->second;
    }

    static SymbolsOrError append_prefix(
        const TaskDataSymbol& symbol,
        const TaskDataSymbol& prefix,
        std::string_view self_name,
        std::function<TaskDerivedConstPtr(std::string_view)> get_raw,
        std::function<SymbolsOrError(const TaskList&)> compile_tasklist);

    bool is_name() const noexcept { return m_symbol == Name; }

    bool is_sharp_type() const noexcept { return is_sharp_type(m_symbol); }

    Type type() const noexcept { return m_symbol; }

    // const std::string& repr() const& noexcept { return repr(m_symbol); }
    const std::string& name() const& noexcept { return is_name() ? m_name : repr(m_symbol); }
};
} // namespace asst
