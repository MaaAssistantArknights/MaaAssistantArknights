#pragma once
#include "Config/AbstractConfig.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include <ranges>
#include <unordered_map>
#include <unordered_set>

namespace asst
{
class BattleDataConfig final : public MAA_NS::SingletonHolder<BattleDataConfig>, public AbstractConfig
{
public:
    virtual ~BattleDataConfig() override = default;

    static inline const std::string& EmptyId = "";

    const std::string get_id(const std::string& name) const
    {
        if (const auto& props = find_oper(name); props != nullptr) {
            return props->id;
        }
        return EmptyId;
    }

    const std::string get_id(battle::Role role, const std::string& name) const
    {
        if (const auto& props = find_oper(role, name); props != nullptr) {
            return props->id;
        }
        return EmptyId;
    }

    const std::shared_ptr<battle::OperProps> find_oper(const std::string& name) const
    {
        if (name.empty()) {
            return nullptr;
        }

        // compatible with old logic: if multiple roles have the same name, return the first one found in m_chars.
        auto it = std::ranges::find_if(m_chars, [&name](const auto& pair) { return pair.second->name == name; });
        if (it != m_chars.cend()) {
            return it->second;
        }
        return nullptr;
    }

    const std::shared_ptr<battle::OperProps> find_oper(battle::Role role, const std::string& name) const
    {
        if (name.empty()) {
            return nullptr;
        }
        auto role_it = m_chars_by_role.find(role);
        if (role_it == m_chars_by_role.cend()) {
            return nullptr;
        }
        auto oper_it =
            std::ranges::find_if(role_it->second, [&name](const auto& pair) { return pair.second->name == name; });
        if (oper_it != role_it->second.cend()) {
            return oper_it->second;
        }
        return nullptr;
    }

    const std::shared_ptr<battle::OperProps> find_oper_by_id(const std::string& id) const
    {
        if (id.empty()) {
            return nullptr;
        }

        auto it = m_chars.find(id);
        if (it != m_chars.cend()) {
            return it->second;
        }
        return nullptr;
    }

    battle::Role get_role(const std::string& name) const
    {
        if (name.empty()) {
            return battle::Role::Unknown;
        }

        // compatible with old logic: if multiple roles have the same name, return the first one found in m_chars.
        const auto& oper_it = find_oper(name);
        if (oper_it) {
            return oper_it->role;
        }
        return battle::Role::Unknown;
    }

    int get_rarity(battle::Role role, const std::string& name) const
    {
        if (const auto& oper = find_oper(role, name); oper != nullptr) {
            return oper->rarity;
        }
        return 0;
    }

    // Legacy wrapper (name-only). If name is ambiguous across roles, returns 0.
    int get_rarity(const std::string& name) const
    {
        if (const auto& oper = find_oper(name); oper != nullptr) {
            return oper->rarity;
        }
        return 0;
    }

    battle::LocationType get_location_type(battle::Role role, const std::string& name) const
    {
        if (const auto& props = find_oper(role, name); props != nullptr) {
            return props->location_type;
        }
        return battle::LocationType::Invalid;
    }

    // Legacy wrapper (name-only). If name is ambiguous across roles, returns LocationType::Invalid.
    battle::LocationType get_location_type(const std::string& name) const
    {
        if (const auto& props = find_oper(name); props != nullptr) {
            return props->location_type;
        }
        return battle::LocationType::Invalid;
    }

    static inline const battle::AttackRange& EmptyRange { { 0, 0 } };

    const battle::AttackRange& get_range(battle::Role role, const std::string& name, size_t index) const
    {
        const auto& props = find_oper(role, name);
        if (props == nullptr) {
            return EmptyRange;
        }
        const auto& ranges = props->ranges;
        if (index >= ranges.size()) {
            if (ranges.empty()) {
                return EmptyRange;
            }
            index = 0;
        }

        const std::string& range_name = ranges.at(index);
        auto range_iter = m_ranges.find(range_name);
        if (range_iter == m_ranges.cend()) {
            return EmptyRange;
        }
        return range_iter->second;
    }

    // Legacy wrapper (name-only). If name is ambiguous across roles, returns EmptyRange.
    const battle::AttackRange& get_range(const std::string& name, size_t index) const
    {
        return get_range(get_role(name), name, index);
    }

    const std::vector<std::string>& get_tokens(battle::Role role, const std::string& name) const
    {
        if (const auto& props = find_oper(role, name); props != nullptr) {
            return props->tokens;
        }
        static const std::vector<std::string> Empty;
        return Empty;
    }

    // Legacy wrapper (name-only). If name is ambiguous across roles, returns empty token list.
    const std::vector<std::string>& get_tokens(const std::string& name) const
    {
        return get_tokens(get_role(name), name);
    }

    bool is_name_invalid(battle::Role role, const std::string& name) const
    {
        return name.empty() || find_oper(role, name) == nullptr;
    }

    // Legacy wrapper (name-only). If name is ambiguous across roles, returns true.
    bool is_name_invalid(const std::string& name) const { return name.empty() || find_oper(name) == nullptr; }

    const std::unordered_set<std::string>& get_all_oper_names() const noexcept { return m_opers; }

    const std::unordered_map<std::string, std::shared_ptr<battle::OperProps>>& get_all_chars() const noexcept
    {
        return m_chars;
    }

    const std::unordered_set<std::string>& get_drones_confusing() const noexcept { return m_drones_confusing; }

protected:
    virtual bool parse(const json::value& json) override;

private:
    std::map<battle::Role, std::unordered_map<std::string, std::shared_ptr<battle::OperProps>>>
        m_chars_by_role;                                                         // role -> (name -> oper)
    std::unordered_map<std::string, std::shared_ptr<battle::OperProps>> m_chars; // id -> oper

    std::unordered_map<std::string, battle::AttackRange> m_ranges;
    std::unordered_set<std::string> m_opers;
    std::unordered_set<std::string> m_drones_confusing; // confused summons: multiple summons of same oper
};

inline static auto& BattleData = BattleDataConfig::get_instance();
} // namespace asst
