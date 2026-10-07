#pragma once

#include "Config/AbstractConfigWithTempl.h"

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace asst
{
class ItemConfig final : public MAA_NS::SingletonHolder<ItemConfig>, public AbstractConfigWithTempl
{
public:
    using Formula = std::unordered_map<std::string, int>;

    virtual ~ItemConfig() override = default;

    const std::string& get_item_name(const std::string& id) const noexcept
    {
        if (id.empty()) {
            static const std::string unknown = "Unknown";
            return unknown;
        }
        if (auto iter = m_item_name.find(id); iter != m_item_name.cend()) {
            return iter->second;
        }
        else {
            static const std::string empty;
            return empty;
        }
    }

    std::optional<int> get_item_rarity(const std::string& id) const noexcept
    {
        if (auto iter = m_item_rarity.find(id); iter != m_item_rarity.cend()) {
            return iter->second;
        }
        return std::nullopt;
    }

    const auto& get_all_item_id() const noexcept { return m_all_item_id; }

    const std::unordered_map<std::string, int>& get_record_exp_items() const noexcept { return m_record_exp_items; }

    std::optional<int> get_record_exp(const std::string& id) const noexcept
    {
        if (const auto item = m_record_exp_items.find(id); item != m_record_exp_items.cend()) {
            return item->second;
        }
        return std::nullopt;
    }

    virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override
    {
        return get_all_item_id();
    }

    const auto& get_ordered_material_item_id() const noexcept { return m_ordered_material_item_id; }

    const auto& get_ordered_non_chip_formula_item_id() const noexcept { return m_ordered_non_chip_formula_item_id; }

    const auto& get_non_chip_material_item_id() const noexcept { return m_non_chip_material_item_id; }

    const Formula& get_item_formula(const std::string& id) const noexcept
    {
        if (auto iter = m_item_formulas.find(id); iter != m_item_formulas.cend()) {
            return iter->second;
        }
        static const Formula empty;
        return empty;
    }

protected:
    virtual bool parse(const json::value& json) override;
    void clear();

    // key：材料编号Id，value：材料名（对应客户端材料名称，utf8）
    std::unordered_map<std::string, std::string> m_item_name;
    std::unordered_map<std::string, int> m_item_rarity;
    std::unordered_map<std::string, Formula> m_item_formulas;
    std::unordered_map<std::string, int> m_record_exp_items;
    std::unordered_set<std::string> m_all_item_id;
    std::vector<std::string> m_ordered_material_item_id;
    std::vector<std::string> m_ordered_non_chip_formula_item_id;
    std::vector<std::string> m_non_chip_material_item_id;
};

inline static auto& ItemData = ItemConfig::get_instance();
}
