#include "ItemConfig.h"

#include <limits>
#include <meojson/json.hpp>
#include <ranges>
#include <string_view>

#include "Utils/Logger.hpp"

namespace
{
constexpr std::string_view ChipCatalystItemId = "32001";
}

bool asst::ItemConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();
    std::unordered_map<std::string, int> material_sortid;
    std::unordered_set<std::string> non_chip_formula_item_ids;
    for (const auto& [id, item_json] : json.as_object()) {
        std::string name = item_json.at("name").as_string();
        m_item_name.emplace(id, std::move(name));
        m_all_item_id.emplace(id);
        if (const auto gain_exp = item_json.find("gainExp")) {
            if ((id != "2001" && id != "2002" && id != "2003" && id != "2004") || !gain_exp->is<int>() ||
                gain_exp->as_integer() <= 0 || gain_exp->as_integer() > std::numeric_limits<int>::max() ||
                item_json.at("classifyType").as_string() != "MATERIAL") {
                LogError << __FUNCTION__ << "Invalid battle record EXP" << id;
                return false;
            }
            m_record_exp_items.emplace(id, gain_exp->as_integer());
        }
        if (auto rarity_opt = item_json.find<int>("rarity")) {
            m_item_rarity.emplace(id, *rarity_opt);
        }
        if (const auto formula_opt = item_json.find("formula")) {
            if (!formula_opt->is_object() || formula_opt->as_object().empty()) {
                LogError << __FUNCTION__ << "invalid item formula" << id;
                return false;
            }
            Formula formula;
            for (const auto& [ingredient, quantity] : formula_opt->as_object()) {
                if (!quantity.is<int>() || quantity.as_integer() <= 0 || !json.contains(ingredient)) {
                    LogError << __FUNCTION__ << "invalid item formula ingredient" << id << ingredient;
                    return false;
                }
                formula.emplace(ingredient, quantity.as_integer());
            }
            m_item_formulas.emplace(id, std::move(formula));
        }
        if (item_json.at("classifyType").as_string() == "MATERIAL") {
            material_sortid.emplace(id, item_json.at("sortId").as_integer());
            // All dual-chip formulas in the current official data consume chip catalysts.
            if (const auto formula_opt = item_json.find("formula");
                formula_opt && !formula_opt->contains(ChipCatalystItemId.data())) {
                non_chip_formula_item_ids.emplace(id);
            }
        }
    }

    m_ordered_material_item_id.clear();
    m_ordered_material_item_id.reserve(material_sortid.size());
    std::ranges::copy(material_sortid | std::views::keys, std::back_inserter(m_ordered_material_item_id));
    std::ranges::sort(m_ordered_material_item_id, std::less {}, [&](const std::string& name) -> int {
        return material_sortid[name];
    });

    m_ordered_non_chip_formula_item_id.clear();
    std::ranges::copy_if(
        m_ordered_material_item_id,
        std::back_inserter(m_ordered_non_chip_formula_item_id),
        [&](const std::string& id) { return non_chip_formula_item_ids.contains(id); });

    // 培养材料识别范围由配方数据决定，不随当前可用刷取关卡改变。
    auto non_chip_material_item_ids = non_chip_formula_item_ids;
    for (const auto& id : m_ordered_non_chip_formula_item_id) {
        for (const auto& [ingredient, quantity] : get_item_formula(id)) {
            if (material_sortid.contains(ingredient)) {
                non_chip_material_item_ids.emplace(ingredient);
            }
        }
    }
    std::ranges::copy_if(
        m_ordered_material_item_id,
        std::back_inserter(m_non_chip_material_item_id),
        [&](const std::string& id) { return non_chip_material_item_ids.contains(id); });

    return true;
}

void asst::ItemConfig::clear()
{
    m_item_name.clear();
    m_item_rarity.clear();
    m_item_formulas.clear();
    m_record_exp_items.clear();
    m_all_item_id.clear();
    m_ordered_material_item_id.clear();
    m_ordered_non_chip_formula_item_id.clear();
    m_non_chip_material_item_id.clear();
}
