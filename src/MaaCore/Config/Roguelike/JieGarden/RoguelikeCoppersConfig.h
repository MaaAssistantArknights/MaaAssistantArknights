#pragma once

#include "Config/AbstractConfig.h"
#include "Utils/SingletonHolder.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace asst
{
enum class CopperRarity
{
    Low,
    Mid,
    High
};

enum class CopperType
{
    Unknown = -1, // 未知类型
    Li = 0,       // 厉钱 - 激进，效果难以捉摸
    Heng = 1,     // 衡钱 - 效果较为均衡
    Hua = 2       // 花钱 - 能带来稳定收益
};

struct RoguelikeCopper
{
    std::string name;
    CopperRarity rarity = CopperRarity::Low;
    CopperType type = CopperType::Unknown;
    int pickup_priority = 0;        // 拾取优先级
    int discard_priority = 1000;    // 丢弃优先级
    int cast_discard_priority = -1; // 可选，已投出时的丢弃优先级，优先级>=0且is_cast=true时替代discard_priority
    int col = 0;
    int index = 0;
    bool is_cast = false;

    int get_copper_discard_priority() const
    {
        if (is_cast && cast_discard_priority >= 0) {
            return cast_discard_priority;
        }
        return discard_priority;
    }
};

class RoguelikeCoppersConfig final : public SingletonHolder<RoguelikeCoppersConfig>, public AbstractConfig
{
public:
    ~RoguelikeCoppersConfig() override = default;

    const std::vector<RoguelikeCopper>& get_coppers(const std::string& theme) const
    {
        static const std::vector<RoguelikeCopper> empty;
        auto it = m_coppers.find(theme);
        return it != m_coppers.end() ? it->second : empty;
    }

    static CopperType get_type_from_template(std::string_view templ_name)
    {
        using enum CopperType;
        if (templ_name == "JieGarden@Roguelike@CoppersTypeLi.png") {
            return Li;
        }
        if (templ_name == "JieGarden@Roguelike@CoppersTypeHeng.png") {
            return Heng;
        }
        if (templ_name == "JieGarden@Roguelike@CoppersTypeHua.png") {
            return Hua;
        }
        return Heng;
    }

    static CopperType get_type_from_name(std::string_view name)
    {
        using enum CopperType;
        if (name.starts_with("厉-")) {
            return Li;
        }
        if (name.starts_with("衡-") || name.find("大炎通宝") != std::string::npos) {
            return Heng;
        }
        if (name.starts_with("花-")) {
            return Hua;
        }
        return Unknown;
    }

    // 根据名称查找通宝信息
    std::optional<RoguelikeCopper> find_copper(std::string_view theme, std::string_view name) const
    {
        const auto& coppers = get_coppers(std::string(theme));
        for (const auto& copper : coppers) {
            if (std::string(name).find(copper.name) != std::string::npos) {
                return copper;
            }
        }
        return std::nullopt;
    }

private:
    bool parse(const json::value& json) override;
    void clear();

    std::unordered_map<std::string, std::vector<RoguelikeCopper>> m_coppers;
};

inline static auto& RoguelikeCoppers = RoguelikeCoppersConfig::get_instance();
}
