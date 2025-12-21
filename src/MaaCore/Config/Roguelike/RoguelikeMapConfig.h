#pragma once

#include <unordered_map>

#include "Config/AbstractConfig.h"

namespace asst
{
enum class RoguelikeNodeType
{
    Init = -1,    // Init
    Unknown = 0,  // 未知
    CombatOps,    // 作战
    EmergencyOps, // 紧急作战
    DreadfulFoe,  // 险路恶敌
    Encounter,    // 不期而遇
    Boons,        // 古堡馈赠/得偿所愿
    SafeHouse,    // 安全的角落
    Recreation,   // 幕间余兴/兴致盎然
    RogueTrader,  // 诡意行商
    // –––––––– 水月主题引入 ––––––––––––––––––––––––
    RegionalCommissions, // 地区委托
    LostAndFound,        // 风雨际会/失与得
    Scout,               // 紧急运输/先行一步
    BoskyPassage,        // 误入奇境/树篱之途
    // –––––––– 萨米主题引入 ––––––––––––––––––––––––
    Prophecy,          // 命运所指
    MysteriousPresage, // 诡秘的预感
    FerociousPresage,  // 凶戾的预感
    // –––––––– 萨卡兹主题引入 ––––––––––––––––––––––––
    IdeaFilter, // 去违存真
    FaceOff,    // 狭路相逢
    // –––––––– 界园岁兽残识引入 ––––––––––––––––––––––––
    Legend,    // 传说
    Omissions, // 拾遗
    Doubts,    // 杂疑
    Disaster,  // 祸乱
    Playtime,  // 常乐
    OldShop,   // 故肆
    YiTrader,  // 易与
    Scheme,    // 筹谋
};

class RoguelikeMapConfig final : public MAA_NS::SingletonHolder<RoguelikeMapConfig>, public AbstractConfig
{
public:
    virtual ~RoguelikeMapConfig() override = default;

    RoguelikeNodeType templ2type(const std::string& theme, const std::string& templ_name) const;
    static std::string type2name(RoguelikeNodeType type);
    static RoguelikeNodeType name2type(const std::string& type_name);

private:
    virtual bool parse(const json::value& json) override;

    std::unordered_map<std::string, std::unordered_map<std::string, RoguelikeNodeType>> m_templ_type_mappings;
};

inline static auto& RoguelikeMapInfo = RoguelikeMapConfig::get_instance();
}
