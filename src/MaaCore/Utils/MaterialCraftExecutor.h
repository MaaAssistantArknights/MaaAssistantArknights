#pragma once

#include <optional>

#include "MaterialCraftPlanner.h"

namespace asst
{
/// Executes against freshly observed recipe quantities; depot snapshots are never consulted.
class MaterialCraftExecutor
{
public:
    struct Actions
    {
        std::function<std::optional<MaterialInventory>(const MaterialFormula&)> read;
        // Returns the number of batches actually completed, including game/operator limits.
        std::function<std::optional<int>(const MaterialCraftOperation&)> execute;
        std::function<bool()> cancelled;
    };

    MaterialCraftExecutor(std::vector<MaterialFormula> formulas, Actions actions);
    bool craft(const MaterialAmount& target);

    const std::string& error() const noexcept { return m_error; }

private:
    bool craft(const MaterialAmount& target, std::unordered_set<std::string>& stack);
    bool fail(std::string error);
    bool cancelled() const;
    std::vector<MaterialFormula> m_formulas;
    Actions m_actions;
    std::string m_error;
};
}
