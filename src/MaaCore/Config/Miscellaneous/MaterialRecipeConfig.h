#pragma once

#include <shared_mutex>

#include "Config/AbstractConfig.h"
#include "MaaUtils/SingletonHolder.hpp"
#include "Utils/MaterialCraftPlanner.h"

namespace asst
{
class MaterialRecipeConfig final : public MAA_NS::SingletonHolder<MaterialRecipeConfig>, public AbstractConfig
{
public:
    std::vector<MaterialFormula> formulas() const;
    std::vector<std::string> item_ids() const;

protected:
    bool parse(const json::value& json) override;

private:
    mutable std::shared_mutex m_mutex;
    std::map<std::string, MaterialFormula> m_formulas;
};

inline auto& MaterialRecipes = MaterialRecipeConfig::get_instance();

MaterialCraftRequest parse_material_craft_request(const json::value& params);
json::value material_craft_plan_json(const MaterialCraftPlan& plan);
}
