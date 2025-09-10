#pragma once

#include <string>

namespace asst
{
enum class RoguelikeNodeType
{
    Init = -1,        // Init
    Unknown = 0,      // 未知
    CombatOps = 1,    // 作战
    EmergencyOps = 2, // 紧急作战
    DreadfulFoe = 3,  // 险路恶敌
    Encounter = 4,    // 不期而遇
    Boons = 5,        // 古堡馈赠/得偿所愿
    SafeHouse = 6,    // 安全的角落
    Recreation = 7,   // 幕间余兴/兴致盎然
    RogueTrader = 8,  // 诡意行商
    // –––––––– 水月主题引入 ––––––––––––––––––––––––
    RegionalCommissions = 9, // 地区委托
    LostAndFound = 10,       // 风雨际会/失与得
    Scout = 11,              // 紧急运输/先行一步
    BoskyPassage = 12,       // 误入奇境/树篱之途
    // –––––––– 萨米主题引入 ––––––––––––––––––––––––
    Prophecy = 13,          // 命运所指
    MysteriousPresage = 14, // 诡秘的预感
    FerociousPresage = 15,  // 凶戾的预感
    // –––––––– 萨卡兹主题引入 ––––––––––––––––––––––––
    IdeaFilter = 16, // 去违存真
    FaceOff = 17,    // 狭路相逢
    // –––––––– 界园树洞引入 ––––––––-–––––––––––––––
    Legend = 18,    // 传说
    Omissions = 19, // 拾遗
    Doubts = 20,    // 杂疑
    Disaster = 21,  // 祸乱
    Playtime = 22,  // 常乐
    OldShop = 23,   // 故肆
    YiTrader = 24,  // 易与
    Scheme = 25     // 筹谋
};

/// <summary>
/// 根据字符串获取节点类型
/// </summary>
/// <param name="node_name">节点类型名称，例如 "YiTrader"</param>
/// <returns>对应的节点类型，如果未找到返回 Unknown</returns>
RoguelikeNodeType name2type(const std::string& node_name);

/// <summary>
/// 获取节点类型的字符串名称
/// </summary>
/// <param name="node_type">节点类型</param>
/// <returns>对应的字符串名称，如果类型未知返回空字符串</returns>
std::string type2name(RoguelikeNodeType node_type);

} // namespace asst
