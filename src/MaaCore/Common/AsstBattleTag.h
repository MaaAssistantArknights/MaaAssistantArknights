#pragma once

#include <compare>
#include <functional>
#include <string>
#include <unordered_map>

namespace asst::battle
{
enum class Role
{
    Unknown,
    Pioneer, // 先锋
    Warrior, // 近卫
    Tank,    // 重装
    Sniper,  // 狙击
    Caster,  // 术士
    Medic,   // 医疗
    Support, // 辅助
    Special, // 特种
    Drone    // 无人机
};
} // namespace asst::battle

namespace asst
{
inline std::string enum_to_string(asst::battle::Role role, bool en = false)
{
    using asst::battle::Role;
    static const std::unordered_map<Role, std::pair<std::string, std::string>> RoleToName {
        { Role::Warrior, { "近卫", "Warrior" } }, { Role::Pioneer, { "先锋", "Pioneer" } },
        { Role::Medic, { "医疗", "Medic" } },     { Role::Tank, { "重装", "Tank" } },
        { Role::Sniper, { "狙击", "Sniper" } },   { Role::Caster, { "术师", "Caster" } },
        { Role::Support, { "辅助", "Support" } }, { Role::Special, { "特种", "Special" } },
        { Role::Drone, { "无人机", "Drone" } },
    };

    if (auto iter = RoleToName.find(role); iter != RoleToName.end()) {
        return en ? iter->second.second : iter->second.first;
    }

    return "Unknown";
}
} // namespace asst

namespace asst::battle
{
struct OperNameTag
{
    Role role = Role::Unknown; // 干员职业
    std::string name;          // 干员名

    auto operator<=>(const OperNameTag&) const = default;

    std::string to_string() const { return "(" + enum_to_string(role) + ", " + name + ")"; }

    explicit operator std::string() const { return to_string(); }
};
} // namespace asst::battle

namespace std
{
template <>
struct hash<asst::battle::OperNameTag>
{
    std::size_t operator()(const asst::battle::OperNameTag& k) const noexcept
    {
        return std::hash<std::string> {}(k.name) ^ (std::hash<int> {}(static_cast<int>(k.role)) << 1);
    }
};
} // namespace std
