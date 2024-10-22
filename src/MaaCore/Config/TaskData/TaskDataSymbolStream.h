#pragma once

#include <optional>
#include <vector>

#include "Config/TaskData.h"
#include "TaskDataSymbol.h"
#include "TaskDataTypes.h"

namespace asst
{
class TaskDataSymbolStream final
{
public:
    using Symbol = TaskDataSymbol;
    using Symbols = TaskDataSymbol::Symbols;
    using SymbolsOrError = TaskDataSymbol::SymbolsOrError;
    using AppendPrefixFunc = std::function<SymbolsOrError(const Symbol&, const Symbol&)>;

private:
    Symbols m_symbolstream;

public:
    ResultOrError<bool> parse(std::string_view task_expr);
    SymbolsOrError decode(AppendPrefixFunc append_prefix, std::string_view self_name) const;
};
} // namespace asst
