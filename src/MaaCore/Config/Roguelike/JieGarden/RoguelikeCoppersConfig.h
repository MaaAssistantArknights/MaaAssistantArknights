#pragma once

#include "Config/AbstractConfig.h"
#include "Utils/SingletonHolder.hpp"

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
    CopperRarity rarity;
    CopperType type;
    int pickup_priority = 0;     // 拾取优先级
    int discard_priority = 2000; // 丢弃优先级

    int col = 0;
    int index = 0;

    RoguelikeCopper() = default;

    RoguelikeCopper(std::string _name, CopperType _type, int col, int index, const std::string& theme);
};

class RoguelikeCoppersConfig final : public SingletonHolder<RoguelikeCoppersConfig>, public AbstractConfig
{
public:
    ~RoguelikeCoppersConfig() override = default;

    const auto& get_coppers(const std::string& theme) const noexcept { return m_coppers.at(theme); }

    static CopperType templ2type(const std::string_view templ_name) noexcept
    {
        if (templ_name == "JieGarden@Roguelike@CoppersTypeLi.png") {
            return CopperType::Li;
        }
        if (templ_name == "JieGarden@Roguelike@CoppersTypeHeng.png") {
            return CopperType::Heng;
        }
        if (templ_name == "JieGarden@Roguelike@CoppersTypeHua.png") {
            return CopperType::Hua;
        }
        return CopperType::Heng; // default
    }

private:
    bool parse(const json::value& json) override;

    void clear();

    std::unordered_map<std::string, std::vector<RoguelikeCopper>> m_coppers;
};

inline static auto& RoguelikeCoppers = RoguelikeCoppersConfig::get_instance();
}
