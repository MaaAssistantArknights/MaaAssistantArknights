#pragma once

#include "Config/AbstractConfig.h"
#include "MaaUtils/SingletonHolder.hpp"

#include <meojson/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace asst
{
// 通宝稀有度枚举
enum class CopperRarity
{
    NONE = -1, // 无稀有度
    NORMAL,    // 普通
    RARE,      // 稀有
    SUPER_RARE // 超稀有
};

// 通宝类型枚举，表示三种不同的通宝类型
enum class CopperType
{
    Unknown = -1, // 未知类型
    Li = 0,       // 厉钱 - 激进，效果难以捉摸
    Heng = 1,     // 衡钱 - 效果较为均衡
    Hua = 2       // 花钱 - 能带来稳定收益
};

// 通宝信息结构体，存储单个通宝的完整信息
struct RoguelikeCopper
{
    std::string name;                         // 通宝名称
    CopperRarity rarity = CopperRarity::NONE; // 稀有度
    CopperType type = CopperType::Unknown;    // 类型
    int pickup_priority = 0;                  // 拾取优先级，数值越高越优先拾取
    int discard_priority = 1000;              // 丢弃优先级，数值越高越优先被丢弃
    int cast_discard_priority = -1;           // 已投出时的丢弃优先级，优先级>=0且is_cast=true时替代discard_priority
    int col = 0;                              // 在列表中的列位置
    int row = 0;                              // 在列表中的行位置
    bool is_cast = false;                     // 是否已投出

    // 获取实际的丢弃优先级，已投出状态下使用特殊优先级
    int get_copper_discard_priority() const
    {
        if (is_cast && cast_discard_priority >= 0) {
            return cast_discard_priority;
        }
        return discard_priority;
    }
};

// RoguelikeCoppersConfig 类管理界园肉鸽通宝的配置信息
class RoguelikeCoppersConfig final : public MAA_NS::SingletonHolder<RoguelikeCoppersConfig>, public AbstractConfig
{
public:
    ~RoguelikeCoppersConfig() override = default;

    // 获取指定主题的所有通宝列表
    const std::vector<RoguelikeCopper>& get_coppers(const std::string& theme) const
    {
        static const std::vector<RoguelikeCopper> empty;
        auto it = m_coppers.find(theme);
        return it != m_coppers.end() ? it->second : empty;
    }

    // 根据模板文件名获取通宝类型
    static CopperType get_type_from_template(std::string_view templ_name)
    {
        static const std::unordered_map<std::string, CopperType> template_map = {
            { "JieGarden@Roguelike@CoppersTypeLi.png", CopperType::Li },
            { "JieGarden@Roguelike@CoppersTypeHeng.png", CopperType::Heng },
            { "JieGarden@Roguelike@CoppersTypeHua.png", CopperType::Hua }
        };
        auto it = template_map.find(std::string(templ_name));
        return it != template_map.end() ? it->second : CopperType::Unknown;
    }

    // 根据通宝名称获取通宝类型
    static CopperType get_type_from_name(std::string_view name)
    {
        if (name.starts_with("厉-")) {
            return CopperType::Li;
        }
        if (name.starts_with("衡-") || name.find("大炎通宝") != std::string::npos) {
            return CopperType::Heng;
        }
        if (name.starts_with("花-")) {
            return CopperType::Hua;
        }
        return CopperType::Unknown;
    }

    // 根据名称查找通宝信息，支持模糊匹配
    std::optional<RoguelikeCopper> find_copper(std::string_view theme, std::string_view name) const
    {
        const auto& coppers = get_coppers(std::string(theme));
        for (const auto& copper : coppers) {
            if (name.find(copper.name) != std::string_view::npos) {
                return copper;
            }
        }

        return std::nullopt;
    }

private:
    struct CoppersConfig
    {
        std::string name;               // 通宝名称
        std::string desc;               // 通宝效果描述（仅作注释，不影响程序运行）
        std::string rarity;             // 稀有度：NONE/NORMAL/RARE/SUPER_RARE（仅作注释，不影响程序运行）
        int pickup_priority = 0;        // 拾取优先级，在掉落选择时使用，数值越高越优先拾取
        int discard_priority = 1000;    // 丢弃优先级，在交换时使用，数值越高越优先被丢弃
        int cast_discard_priority = -1; // 可选字段，已投出时的丢弃优先级，仅在已投出且值>=0时替代 discard_priority;
                                        // 这通常适用于一些投出后效果发生变化的通宝 (如变为大炎通宝的通宝)

        MEO_JSONIZATION(name, MEO_OPT desc, rarity, pickup_priority, discard_priority, MEO_OPT cast_discard_priority);
    };

    // 解析JSON配置文件
    bool parse(const json::value& json) override;

    // 清空配置数据
    void clear();

    // 存储按主题分组的通宝配置数据
    std::unordered_map<std::string, std::vector<RoguelikeCopper>> m_coppers;
};

// 全局单例实例访问器
inline static auto& RoguelikeCoppers = RoguelikeCoppersConfig::get_instance();
}
